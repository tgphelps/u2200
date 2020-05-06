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
          call      dofile
          jgd       r4,loop1

          call      summary
          er        exit$

err1
.         a$print   (0103,('open failed'l))
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

. Read the file header, verify that the label is good.
. Save the sector address of the first file record.
. Return the file count.

openextract
          beginsub
          pushregs  x5
          call      openmfd
          lx        x5,a0             . x5 -> sector 0
          la        a0,mflabl,x5
          te        a0,($cfs('*MFDB*')) . look like an MFD extract?
          j         openerr2
          top       a15,(option('D'))  . if debug on
          j         open01
          la        a0,mfflad,x5
          sa        a0,cursect         . remember where to start files
          a$edit    edpkt
          a$emsg    msgfilect
          a$edecv   mfflct,x5
          a$eprint
open01
          la        a0,iobuff+1
          popregs
          endsub

openerr2
          l$snap    'a0',2
          aprint    'ERROR: Invalid MFD extract header'
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
$(2)
dofilepkt $res      5

. Read current record. It must be a file record.
. Read next record until you find another file record.

dofile
          beginsub
          aprint    'doing file...'
          call      readcur           . a0 -> record
          tnz,h1    fqual,a0          . better not be zero
          er        err$
          la,u      a1,dofilepkt
          dl        a2,fqual,a0
          ds        a2,fqual,a1
          dl        a2,fname,a0
          ds        a2,fname,a1
          aprint    'got file record'
          . print dofilepkt

dofile01  . loop over DAD tables
          call      readnext
          tz,h1     fqual,a0          . if zero, it's a DAD table
          j         dofile10
          aprint    'got DAD table'
          j         dofile01
dofile10
          endsub

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
          tz,s1     iopkt+3
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
          . aprint    'readnext'
          inc       cursect
          nop
          call      readcur           . a0 -> record
          endsub



. If the 'cursect' is not loaded, fetch the track that contains it.
. Then return a pointer to the requested sector.

readcur
          beginsub
          . aprint    'readcur'
          top       a15,(option('D'))
          j         readc01
          a$edit    edpkt
          a$emsg    ('read &/&'ld)
          a$edecv   cursect
          a$emsgr
          a$edecv   curbuff
          a$eprint
readc01
          la        a0,cursect
          ssl       a0,6
          lssl      a0,6
          tne       a0,curbuff
          j         readgotit
          . a0 = sector address we want to read
          sa        a0,iopkt+5
          push      a0
          top       a15,(option('D'))
          j         readc03
          pop       a0
          sa        a0,curbuff        . remember what track we loaded
          a$edit    edpkt
          a$emsg    ('I/O :&'ld)
          a$edecv   iopkt+5
          a$eprint
readc03
          la,u      a0,iopkt
          er        iow$
          tz,s1     iopkt+5
          er        err$
readgotit . return a0 -> current sector
          la        a0,cursect
          ana       a0,curbuff        . a0 = sector offset into buffer
          msi,u     a0,28
          aa,u      a0,iobuff         . a0 -> current sector
          endsub


/
$(2)      . Global data
edpkt     a$editpkt 33,edline
edline    $res      33
msgfilect 'File count = &'


          end       start
