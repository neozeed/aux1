# @(#)Copyright Apple Computer 1987\tVersion 1.1 of printcap.pro on 87/05/04 19:10:11
# PostScript printer driven by TranScript software
# PostScript and TranScript are trademarks of Adobe Systems Incorporated
PRINTER|ps|postscript|PostScript:\
	:lp=/dev/PRINTER:sd=SPOOLDIR/PRINTER:\
	:lf=LOGDIR/PRINTER-log:af=ACCTDIR/PRINTER.acct:\
	:br#9600:rw:fc#0000374:fs#0000003:xc#0:xs#0040040:mx#0:sf:sb:\
	:if=PSLIBDIR/psif:\
	:of=PSLIBDIR/psof:gf=PSLIBDIR/psgf:\
	:nf=PSLIBDIR/psnf:tf=PSLIBDIR/pstf:\
	:rf=PSLIBDIR/psrf:vf=PSLIBDIR/psvf:\
	:cf=PSLIBDIR/pscf:df=PSLIBDIR/psdf:
