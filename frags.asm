          $include  'maxr$'
          $include  'tgpdef'
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


$(1)
start
          lx        x4,(1,thestack)   . load stack pointer
          la        a15,a5            . save program options

          call      openextract       . return file count, or 0 on error
          jz        a0,err1

          lr        r4,a0             . r4 = file count
          jgd       r4,$+1
loop1
          call      dofile            . process next file
          jgd       r4,loop1

          call      summary           . do the real processing here
          er        exit$

err1      . Failed to file a good, readable MFD extract.
          aprint    'open failed'
          er        err$


/
$(2)
iopkt     i$od      '$MFDB$',r$ 1792,iobuff 0
iobuff    $res      1792
cursect   $res      1       . next mfd sector we need to look at
curbuff   $res      1       . sector addr of loaded track


$(1)

mflabl    $equf     0
mfflct    $equf     1
mfflad    $equf     4

mfqual    $equf     0
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
ftape     $equf     5
$(2)
filenum   +         0
dofilepkt $res      6
domsg     $cfs('DOING FILE &: &')

. Read current record. It must be a file record.
. Read next record until you find another file record.

dofile
          beginsub
          inc       filenum           . bump file number for msg
          nop

          call      readcur           . a0 -> record
          tnz,h1    fqual,a0          . error if not a file record
          er        err$

          la,u      a1,dofilepkt      . save file info for processing
          sz        ftape,a1
          la        a2,mftype,a0
          tep,u     a2,mmtape         . is this a tape file?
          sp1         ftape,a1          . yes, remember that
          dl        a2,mfqual,a0
          ds        a2,fqual,a1
          dl        a2,mffile,a0
          ds        a2,fname,a1
          la        a2,mfcycl,a0
          sa        a2,fcyc,a1

          e$dit     edpkt             . msg, mostly for debugging
          e$msg     domsg
          e$decv    filenum
          e$msgr
          e$fd2     dofilepkt+fqual
          e$char    $cfs('*')
          e$fd2     dofilepkt+fname
          e$char    $cfs('(')
          e$decv    dofilepkt+4,,h1  . shitty
          e$char    $cfs(')')
          e$print

dofile01  . loop over DAD tables

          call      readnext          . a0 -> following record
          tz,h1     fqual,a0          . if zero, it's a DAD table
          j         dofile10          . it's another file record. Stop
          tz        dofilepkt+5       . XXX was this a tape file?
          j         dofilerr          . that's not good
                                      . a0 -> DAD table
          halt
          aprint    'got DAD table'   . debug
          j         dofile01
dofile10
. XXX somewhere, we need to select only disk files
          endsub

dofilerr
          aprint    'ERROR: Tape file has DAD tables.'
          er        err$
/
$(1)
summary
          beginsub
          aprint    'summary goes right here...'
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
          top       a15,(option('D'))
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
          top       a15,(option('D'))
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
$(2)      . Global data
aedpkt     a$editpkt 33,edline
edpkt      e$ditpkt 22,edline
edline    $res      33
msgfilect 'File count = &'


          end       start
