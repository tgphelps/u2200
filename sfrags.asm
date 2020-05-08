          $include  'maxr$'
          $include  'tgpdef'

          $include  'struc$'
          struc$,'OLD PROGRAM'

          $ascii
          $info     1 3               . quarter-word sensitive

. Version 1
. Just print qual*filenames and DAD counts.
. No selection, no sorting.

. I assume the MFD extact file and sort/merge file XA
. are assigned, because MFDEDT should have just run.
. (We'll polish this later.)



$(2)      $lit
thestack  stackgen  50

idpkt     i$dpkt    'IDBUFF' 'FRAGS 1R1'
mindads    $res      1

$(1)
start
          lx        x4,(1,thestack)   . load stack pointer
          la        a15,a5            . save program options

          i$d       idpkt
          er        err$
          er        aprint$

          call      getparam          . Get parameter from call line
          sa        a0,mindads        . minimum DADs we care about
          call      openextract       . return file count, or 0 on error
          jz        a0,err1

          lr        r4,a0             . r4 = file count
          jgd       r4,$+1
loop1
          call      dofile            . process next file
          jgd       r4,loop1

          . call      summary
          er        exit$

err1      . Failed to file a good, readable MFD extract.
          aprint    'open failed'
          er        err$

/
$(2)
inforbuf  res       50
param     res       2
$(1)

. getparam: Read INFOR table and return the binary value of field 010106

getparam
          beginsub
          i$nfor    'RINF$',50,inforbuf
          j         badinfor
          i$nfor    'SINF$',010106
          j         usage
.         ds        a0,param
.         e$dit     edpkt
.         e$msg     ($cfs('PARAM: &'ld))
.         e$fd2     param
.         e$print
. a0,a1 = field 010106
. a3 = char count
          call      fdbin             . a0 = parameter
          endsub
badinfor
          aprint    'FRAGS must be called as a processor.'
          er        exit$
usage
          aprint    'usage: @FRAGS <mindads>'
          er         exit$
/
$(2)
iopkt     i$od      '$MFDB$',r$ 1792,iobuff 0
iobuff    $res      1792
cursect   $res      1                 . next mfd sector we need to look at
curbuff   $res      1                 . sector addr of loaded track


$(1)

mflabl    $equf     0                 . fields in MFD extract header
mfflct    $equf     1
mfflad    $equf     4

mfqual    $equf     0                 . fields in MFD file record
mffile    $equf     2
mftype    $equf     12,,s6
mmtape    $equ      1
mfcycl    $equf     19,,h2

. Read the file header, verify that the label is good.
. Save the sector address of the first file record.
. Return the file count.

openextract
          beginsub
          pushregs  x5
          call      openmfd           . open and read first track
          lx        x5,a0             . x5 -> sector 0
          la        a0,mflabl,x5
          te        a0,($cfs('*MFDB*')) . look like an MFD extract?
          j         openerr2
          la        a0,mfflad,x5
          sa        a0,cursect         . remember where to start files
          top       a15,(option('D'))  . if debug on
          j         open01
          a$edit    aedpkt
          a$emsg    msgfilect
          a$edecv   mfflct,x5
          a$eprint
open01
          la        a0,iobuff+1        . return file count
          popregs
          endsub

openerr2
          aprint    'ERROR: Invalid MFD extract header'
          l$snap    'a0',2
          er        err$
/
$(1)

. dofile: Return a pointer to a packet that describes the next file
. in the MFD extract. At entry, a call to 'readcur' should fetch
. what should be a file record. DAD records should immediately
. follow it.

. Packet to hold file information:
fqual     $equf     0
fname     $equf     2
fcyc      $equf     4,,h1
fdads     $equf     4,,h2
fholes    $equf     5,,h1             . we don't count holes yet
ftape     $equf     5,,h2
$(2)
filenum   +         0                 . not used any more
dofilepkt $res      6

. Read current record. It must be a file record.
. Read next record until you find another file record.

$(1)
dofile
          beginsub
          pushregs  x5,x6
          inc       filenum           . bump file number for msg
          nop

          call      readcur           . a0 -> record
          tnz,h1    fqual,a0          . error if not a file record
          er        err$

          lx,u      x5,dofilepkt      . save file info for processing
          sz        ftape,x5          . assume not a tape
          sz        fdads,x5          . dads = 0
          sz        fholes,x5         . holes = 0
          la        a2,mftype,a0
          tep,u     a2,mmtape         . is this a tape file?
          sp1       ftape,x5          . yes, remember that
          dl        a2,mfqual,a0
          tne       a2,($cfs('*EOF**')) . should never find the EOF
          er        err$
          ds        a2,fqual,x5       . save qual
          dl        a2,mffile,a0
          ds        a2,fname,x5       . save filename
          la        a2,mfcycl,a0
          sa        a2,fcyc,x5        . save f-cycle


dofile01  . loop over DAD tables

          call      readnext          . a0 -> following record
          tz,h1     fqual,a0          . if zero, it's a DAD table
          j         dofile10          . it's another file record. Stop
          tz        ftape,x5          . was this a tape file?
          j         dofilerr          . that's not good
                                      . a0 -> DAD table
          call      countdads         . returns dad count in this table
          aa        a0,fdads,x5
          sa        a0,fdads,x5       . update total dad count
          . aprint    'got DAD table'   . debug
          j         dofile01
dofile10  . finished reading DAD tables
          tz        ftape,x5          . is this a tape file?
          j         dofile20          . yes, don't print anything
          la        a0,fdads,x5
          tle       a0,mindads        . enough DADs to show?
          j         dofile20          . no -> dofile20
          e$dit     edpkt             . print file, cycle, dad count
          e$decf    3,fdads,x5
          e$char    $cfs(' ')
          e$fd2     fqual,x5
          e$char    $cfs('*')
          e$fd2     fname,x5
          e$char    $cfs('(')
          e$decv    fcyc,x5
          e$char    $cfs(')')
          e$print
dofile20
          popregs
          endsub

dofilerr
          aprint    'ERROR: Tape file has DAD tables.'
          er        err$

. countdads: return a0 = count of DADs in the DAD table a0 points to
. We don't count holes as DADs.

countdads
          beginsub
          pushregs  x5
          lx        x5,a0             . x5 -> DAD table
          tep       a15,(option('D'))
          call      printdadt
          la        a0,x5
          aa,u      a0,4              . point to first DAD
          lr,u      r1,8-1            . max 8 DADs per table
          la,u      a2,0              . DADs so far
dadloop
          la,h2     a1,2,a0           . get LDAT
          top,u     a1,0400000        . is this a hole?
          aa,u      a2,1              . no, bump DAD count
          la,h1     a1,2,a0           . DAD flags
          tep,u     a1,4              . last DAD in this table?
          j         daddone           . yes -> daddone
          aa,u      a0,3              . a0 -> next DAD
          jgd       r1,dadloop
daddone
          la        a0,a2             . return DAD count
          popregs
          endsub

. printdadt: print contents of DAD table
. a0 -> DAD table

printdadt
          beginsub
          pushregs  x5,r5
          lx        x5,a0
          ax,u      x5,4              . x5 -> first DAD
          lr,u      r5,8-1
printd01
          a$edit    aedpkt
          la,h2     a0,2,x5
          tep,u     a0,0400000
          j         printd03
          a$efd1    ('FRAG')
          j         printd04
printd03
          a$efd1    ('HOLE')
printd04
          a$echar   ' '
          a$edecv   1,x5
          a$eprint
          la,h1     a0,2,x5
          tep,u     a0,4              . last DAD?
          j         printd02          . yes
          ax,u      x5,3
          jgd       r5,printd01
printd02
          popregs
          endsub
/
$(1)
summary
          beginsub
          aprint    'do we have any'
          endsub
/
. I/O functions to read the MFD extract file.
. Three functions:
. 1. openmfd  - opens file and sets 'cursect' to 0.
. 2. readcur  - returns a pointer to sector 'cursect'
. 3. readnext - increments 'cursect' and return ptr to that sector.
.
. We read in track-size chunks, and fetch another one when necessary


openmfd
          beginsub
          la,u      a0,iopkt
          er        iow$              . read track 0
          tz,s1     iopkt+3           . good I/O?
          j         openerr1
          sz        cursect           . sector 0 is current
          sz        curbuff           . track 0 is loaded
          call      readcur           . a0 -> sector 0
          endsub

openerr1
          aprint    'ERROR: I/O error reading header'
          er        err$



. Just increment cursect and call readcur

readnext
          beginsub
          inc       cursect           . Make cursect get the next one
          nop
          call      readcur           . a0 -> record
          endsub



. If the 'cursect' is not loaded, fetch the track that contains it.
. Then return a pointer to the requested sector.

readcur
          beginsub
          . top       a15,(option('D'))
          j         readc01
          a$edit    aedpkt
          a$emsg    ('read &/&'ld)
          a$edecv   cursect
          a$emsgr
          a$edecv   curbuff
          a$eprint
readc01
          la        a0,cursect        . needed track = (cursect//64)*64
          ssl       a0,6
          lssl      a0,6
          tne       a0,curbuff        . do we have that one?
          j         readgotit         . yes, no I/O necessary

          . a0 = sector address we want to read
          sa        a0,iopkt+5        . set I/O address
          sa        a0,curbuff        . remember what track we loaded
          . top       a15,(option('D'))
          j         readc03
          a$edit    aedpkt            . debug msg
          a$emsg    ('I/O :&'ld)
          a$edecv   iopkt+5
          a$eprint
readc03
          la,u      a0,iopkt
          er        iow$              . read the track
          tz,s1     iopkt+5           . check I/O status
          er        err$
readgotit . return a0 -> current sector
          la        a0,cursect
          ana       a0,curbuff        . a0 = sector offset into buffer
          msi,u     a0,28             . times words/sector
          aa,u      a0,iobuff         . a0 -> current sector
          endsub

/
$(1)
. I stole this code from MFDEDT. I couldn't bear doing it from scratch.
. a0,a1 = Fieldata representation of a number, LJSF
. a3 = character count

fdbin
          beginsub
          ANA,U     A3,1              . FIND - DECR CHAR COUNT FOR LOOP
          LA,U      A5,0              . INIT ACCUMULATOR = 0
DE052
          LDSC      A0,6              . NEXT CHAR TO A1 LOWER
          AND,U     A1,077            . ISOLATE CHAR IN A2
          ANA,U     A2,$cfs('0')      . CONVERT TO BINARY
          TG,U      A2,0              . CHECK FOR LEGAL RANGE, 0-9
          TG,U      A2,9+1            .
          J         usage
          MSI,U     A5,10             . MULT ACCUMULATOR BY TEN
          AA,U      A5,,A2            .  AND ADD IN NEXT DIGIT
          JGD       A3,DE052          . BACK FOR NEXT DIGIT
          la        a0,a5
          endsub
/
$(2)      . Global data
aedpkt     a$editpkt 33,edline
edpkt      e$ditpkt 22,edline
edline    $res      33
msgfilect 'File count = &'


          end       start
