
          $include  'maxr$'

          $def
          $level    0,1,0

. This element contains whatever general code that I may want
. available in almost every MASM source element that I write.
. Include in other MASM programs via:
.
.         $include  'tgpdef'
.
. NOTE: Any literals generated will go under $(3), so they
. can live in a write-protected ibank.

. General rules that all code will follow are:
.
. 1. Register X4 is the stack pointer, and no code should directly
.    reference X4. There are stack manipulation procs that handle
.    all of that.
. 2. The main program can do whatever he wants with any register,
.    except the stack pointer. It is responsible for allocating
.    the stack, at assembly time, and loading the stack pointer
. 3. All subroutines are called via 'LMJ X11'. Subroutines may
.    destroy any minor set register, but MUST restore any other
.    register to its original contents before it returns.

. Here are the stack manipulation procs:
.
. stackgen
. This proc reserves an area at assembly time of whatever size you
.            need. The label on the stackgen call is the address
.            of a 'RES' area of the proper size, followed by one word
.            containing $CFS('ENDSTK'). There is also
.            one word BEFORE the label that contains the number of
.            words in the stack. Those two extra words can be used
.            to determine whether the stack has overflowed. (There
.            is a proc to do that.)

stackp    $equ      x4

. To use 'stackgen', do this:
. mystack stackgen  50

p         $proc
stackgen* $name
stacksz*  $equ      p(1,1)
          +         stacksz
*         $res      stacksz
          $cfs('ENDSTK')
          $end

. The 'stackchk' proc has no parameters, and can be called at any time
. after the stack pointer has been loaded. It checks that the
. end-of-stack sentinel is intact, and does an ERR$ if it isn't.
. (You might call this before the main program exits, to see if you
. need to debug more. Or you can call it as often as you wish.)

p         $proc     1,5
stackchk* $name
          la        a0,p(1,1)-1
          aa,u      a0,p(1,1)
          la        a0,0,a0
          te        a0,($cfs('ENDSTK'))
          er        err$
          $end

. The 'push' and 'pop' procs do the obvious things with ONE register.
. If you have multiple registers to save, you can call them multiple
. times, or use the 'pushregs' and 'popregs' below.

p         $proc     1,1
push*     $name
          s         p(1,1),0,*stackp
          $end

p         $proc     1,2
pop*      $name
          anx,u     stackp,1
          l         p(1,1),0,stackp
          $end

. 'pushregs' and 'popregs' push and pop as many registers as you
. wish. These might be used at the top and bottom of a subroutine,
. for example. Specify the registers you want to save on the
. 'pushregs' call, and don't specify any on 'popregs'. Popregs will
. automatically pop everything pushed by the last 'pushregs', and
. it will pop them in the reverse order so everything comes out okay.
. (I can never manually keep my pushes and pops in sync, and in the
. right order, so I'm having the procs do it for me.)
. WARNING: These procs assume that 'popregs' call follows the 'pushregs'
. call in the source code. If you jump around in your code so that
. pushregs executes first, but follows popregs in the source code,
. Bad Things Will Happen. So don't get cute.

p         $proc     1
pushregs* $name
stacked*  $equ      $node
stacked(0)* $equ    p(1)
i         $repeat   1,p(1)
stacked(i)* $equ    p(1,i)
          push      p(1,i)
          $endr
          $end

p         $proc     0
popregs*  $name
i         $repeat   stacked(0),1,-1
          pop       stacked(i)
          $endr
          $end

. Function to generate those bit settings that correspond
. to option letters on processor calls. It's the callers job to know
. that options 'A' to 'H' must be in a literal, and the others can
. be immediate values.
. Examples:
.        top,u      a15,option('M')
.        tep        a15,(option('G'))

f         $func
option*   $name
          $end      1*/($cfs('Z')-$cfs(f(1)))

. Proc to print an ASCII string:
.         aprint    'any string'

p         $proc     1
aprint*   $name
$(3)
str       $gen      p(1,1)L
len       $equ      $sl(p(1,1))//4
pcw       +         (0100+len, str)
$($ilcn)
          la        a0,pcw
          er        aprint$
          $end

. Procs to call and return from subroutines

p         $proc     1,1
call*     $name
          lmj       x11,p(1,1)
          $end

p         $proc     0,1
ret*      $name
          j         0,x11
          $end

. I can't consistently remember to save X11 in a subroutine, when
. it is calling another one via X11. Let's automate that.


p         $proc     0,1
beginsub* $name
          push      x11
          $end

p         $proc     0,3
endsub*   $name
          pop       x11
          j         0,x11
          $end

. Proc to generate a debug halt instruction for the PS/2200 debugger

p         $proc     0,1
halt*     $name
          +         001111,0111111
          $end




          $end
