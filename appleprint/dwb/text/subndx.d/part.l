%{
#ifndef lint	/* .../appleprint/dwb/text/subndx.d/part.l */
#define _AC_NAME part_l
#define _AC_MAIN "@(#) Copyright (c) 1984-85 AT&T-IS, All Rights Reserved.  {Apple version 1.2 87/11/11 22:12:21}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987	Version 1.2 of part.l on 87/11/11 22:12:21";
  char *_Version_ = "A/UX Release 1.0";
#endif		/* _AC_HISTORY */
#endif		/* lint */
%}

%{
/*	Copyright (c) 1984 AT&T	*/
%}
%{
/*	  All Rights Reserved  	*/
%}

%{
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
%}
%{
/*	The copyright notice above does not evidence any   	*/
%}
%{
/*	actual or intended publication of such source code.	*/
%}

%{
#define _AC_MODS
#define _AC_MODS
%}

%{
#include "style.h"
#include "names.h"
#include "conp.h"
FILE *deb;
int lineflg = 0;
int nosave = 1;
int part = 0;
int barebones = 0;
int topic = 0;
int style = 1;
int pastyle = 0;
int pstyle = 0;
int sstyle = 0;
int lstyle = 0;
int rstyle = 0;
int estyle = 0;
int nstyle = 0;
int Nstyle = 0;
int sthresh;
int lthresh;
int rthresh;
int nomin;
int penflg = 0;
int pepflg = 0;
int pnpflg = 0;
int penpflg = 0;
char s[SCHAR];
char *sptr = s;
struct ss sent[SLENG];
struct ss *sentp = sent;
float wperc();
float sperc();
float typersent();
float vperc();
int numsent = 0;
int qcount = 0;
int icount = 0;
long vowel = 0;
long numwds = 0;
long twds = 0;
long numnonf = 0;
long letnonf = 0;
int maxsent = 0;
int maxindex = 0;
int minsent = 30;
int minindex = 0;
int simple = 0;
int compound = 0;
int compdx = 0;
int prepc = 0;
int conjc = 0;
int complex = 0;
int tobe = 0;
int adj = 0;
int infin = 0;
int pron = 0;
int passive = 0;
int aux = 0;
int adv = 0;
int verbc = 0;
int tverbc = 0;
int noun = 0;
long numlet = 0;
int beg[15]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
int sleng[50];
int nsleng = 0;
int j,jj,i;
int comma = 0;
int cflg;
int question;
int quote = 0;
char *st;
int initf = 0;
int over = 1;
int nroff = 0;
int nrofflg = 0;
int leng[MAXPAR];
int sentno= 0;
%}
C	[A-DF-Za-z]
%%
^"."$           {
                fflush(stdout);
  }
^{C}:.+	{
collect:
	sentp->cc = sentp->ic = yytext[0];
	if(sentp->cc == NOM)
		sentp->cc = NOUN;
collect1:
	nsleng++;
	sentp->leng = yyleng-2;
	sentp++->sp = sptr;
	if(sentp >= &sent[SLENG-1]){
		if(over)fprintf(stderr,"\nParts (part.l): sentence %d too many words. Only first %d words will be used.\n\n",numsent+1, SLENG-2);
		over=0;
		sentp--;
	}
	if(sptr+yyleng >= &s[SCHAR-1]){
		if(over)fprintf(stderr,"\nParts (part.l): sentence %d too many characters. Only first %d characters \nwill be used.\n\n",numsent+1, SCHAR);
		over=0;
	}
	else {
		for(i=2;i<yyleng;i++)*sptr++=yytext[i];
		*sptr++ = '\0';
	}
	}
^";:".+	{
	sentp->cc=END;
	sentp->ic = ';';
	goto collect1;
	}
^",:".+	{
	comma++;
	goto collect;
	}
^",:"[:,-]+	{
	comma++;
	goto collect;
	}
[\n]	;
^"\":".+	{
	goto collect;
	}
^E:"~."	{
	cflg = 1;
	goto sdone;
	}
^E:.+	{
	cflg = 0;
sdone:
	over=1;
	sentp->cc=sentp->ic=END;
	sentp++->sp = sptr;
	for(i=2;i<yyleng;i++)*sptr++=yytext[i];
	*sptr++='\0';
	if(yytext[2]=='?')question=1;
	else question=0;

fragment:
	jj=0;
	if(quote == 1 && sent[jj].cc == ED){
		sent[jj].cc = VERB;
		quote = 0;
	}
	if(sent[jj].cc=='"')jj++;
	if(sent[jj].cc==SUBCONJ){
		if(sent[jj+1].cc == ','){
			sent[jj].cc=ADV;
			jj += 2;
			comma--;
		}
		else {
			jj=scan(1,',',0);
			if(jj != -1)jj++;
			comma--;
		}
	}
	if(jj != -1){
		if(sent[jj].cc==CONJ || sent[jj].cc=='"')jj++;
		while((jj=scan(jj,END,cflg)) != -1){
			jj++;
			if(sent[jj].cc == SUBCONJ && sent[jj+1].cc == ','){
				sent[jj].cc=ADV;
				jj += 2;
				comma--;
			}
		}
	}
	st = sent[i].sp;
	if(*(st+1) == '"')
		if(*st == '?' || *st == '!')quote = 1;
	outp();
	nsleng = 0;
	if(nroff){
		if(sentno > 0){
			printf(".SL \"");
			for(i=0;i<sentno;i++)
				printf(" %d",leng[i]);
			printf("\"\n");
			sentno = 0;
		}
		printf("%s",&yytext[1]);
		nroff = 0;
	}
	sptr=s;
	sentp=sent;
	comma=0;
	}
;.+	{
	if(style){
		nomin = atoi(&yytext[1]);
	}
	}
^:.+[\n]	{
	if(!lineflg){
		nrofflg=1;
		if(sentp != sent){
			sentp->cc = sentp->ic = END;
			sentp++->sp = sptr;
			*sptr++ = '.';
			*sptr++ = '\0';
			over = 1;
			nroff = 1;
			goto fragment;
		}
		if(sentno > 0){
			printf(".SL \"");
			for(i=0;i<sentno;i++)
				printf(" %d",leng[i]);
			printf("\"\n");
			sentno = 0;
		}
	}
	printf("%s",&yytext[1]);
	}
%%
yywrap(){
	int ii;
	int ml,mg,lsum,gsum;
	float aindex, avl, avw;
	float cindex,kindex,findex,fgrad;
	float syl, avsy, adjs,snonf;
	FILE *io;

	if(style){
	if(numwds == 0 || numsent == 0)exit(0);
	avw = (float)(numwds)/(float)(numsent);
	avl = (float)(numlet)/(float)(numwds);
	aindex = 4.71*avl + .5*avw -21.43;
	syl = .9981*vowel-.3432*twds;
	avsy = syl/twds;
	kindex = 11.8*avsy+.39*avw-15.59;
	findex = 206.835-84.6*avsy-1.015*avw;
	if(findex < 30.)fgrad = 17.;
	else if(findex > 100.) fgrad = 4.;
	else if(findex > 70.)fgrad=(100.-findex)/10 +5.;
	else if(findex > 60.)fgrad =(70.-findex)/10+8.;
	else if(findex >50.)fgrad=(60.-findex)/5+10;
	else fgrad=(50.-findex)/6.66 +13.;
	adjs = 100 * (float)numsent/numwds;
	cindex = 5.89*avl-.3*adjs-15.8;
	printf("readability grades:\n	(Kincaid) %4.1f  (auto) %4.1f  (Coleman-Liau) %4.1f  (Flesch) %4.1f (%4.1f)\n",kindex,aindex,cindex,fgrad,findex);
	printf("sentence info:\n");
	printf("	no. sent %d no. wds %ld\n",numsent,numwds);
	printf("	av sent leng %4.1f av word leng %4.2f\n",avw,avl);
	printf("	no. questions %d no. imperatives %d\n",qcount,icount);
	if(numnonf != 0){
		snonf = (float)(letnonf)/(float)(numnonf);
	}
	printf("	no. content wds %ld  %4.1f%%   av leng %4.2f\n",numnonf,(float)(numnonf)*100/numwds,snonf);
	mg = avw + 10.5;
	if(mg > 49)mg = 49;
	ml = avw - 4.5;
	if(ml <= 0)ml = 1;
	else if(ml > 49)ml=48;
	gsum = lsum = 0;
	for(ii=0;ii<50;ii++){
		if(ii < ml)lsum += sleng[ii];
		else if(ii > mg)gsum+= sleng[ii];
	}
	printf("	short sent (<%d)%3.0f%% (%d) long sent (>%d) %3.0f%% (%d)\n",ml,sperc(lsum),lsum,mg,sperc(gsum),gsum);
	if (minsent == 30 && minindex ==0 ) { minsent=maxsent; minindex=1;}
	/*  30 is the initial value for minsent and 0 for minindex, when there
	is only one sentence these need to be not output in style table*/
	printf("	longest sent %d wds at sent %d; shortest sent %d wds at sent %d\n",maxsent,maxindex,minsent,minindex);
	printf("sentence types:\n");
	printf("	simple %3.0f%% (%d) complex %3.0f%% (%d)\n",sperc(simple),simple,sperc(complex),complex);
	printf("	compound %3.0f%% (%d) compound-complex %3.0f%% (%d)\n",sperc(compound),compound,sperc(compdx),compdx);
	printf("word usage:\n");
	printf("	verb types as %% of total verbs (%d)\n",tverbc);
	printf("	tobe %3.0f%% (%d) aux %3.0f%% (%d) inf %3.0f%% (%d)\n",vperc(tobe),tobe,vperc(aux),aux,vperc(infin),infin);
	if(verbc != 0)adjs = (float)(passive)*100/(float)(verbc);
	else adjs=0;
	printf("	passives as %% of non-inf verbs %3.0f%% (%d)\n",adjs,passive);
	printf("	types as %% of total\n");
	printf("	prep %3.1f%% (%d) conj %3.1f%% (%d) adv %3.1f%% (%d)\n",wperc(prepc),prepc,wperc(conjc),conjc,wperc(adv),adv);
	printf("	noun %3.1f%% (%d) adj %3.1f%% (%d) pron %3.1f%% (%d)\n",wperc(noun),noun,
		wperc(adj),adj,wperc(pron),pron);
	printf("	nominalizations %3.0f %% (%d)\n",wperc(nomin),nomin);
	printf("sentence beginnings:\n");
	ii=beg[0]+beg[7]+beg[6]+beg[3]+beg[8];
	printf("	subject opener: noun (%d) pron (%d) pos (%d) adj (%d) art (%d) tot %3.0f%%\n",
beg[0],beg[7],beg[6],beg[3],beg[8],sperc(ii));
	printf("	prep %3.0f%% (%d) adv %3.0f%% (%d) \n",sperc(beg[9]),beg[9],sperc(beg[4]),beg[4]);
	printf("	verb %3.0f%% (%d) ",sperc(beg[1]+beg[10]+beg[11]),beg[1]+beg[10]+beg[11]);
	printf(" sub_conj %3.0f%% (%d) conj %3.0f%% (%d)\n",sperc(beg[13]),beg[13],sperc(beg[5]),beg[5]);
	printf("	expletives %3.0f%% (%d)\n",sperc(beg[14]),beg[14]);
#ifdef SCATCH
	if(nosave && (fopen(SCATCH,"r")) != NULL){
	if(((io=fopen(SCATCH,"a")) != NULL)){
		fprintf(io," read %4.1f %4.1f %4.1f %4.1f %4.1f",kindex, aindex, cindex, findex, fgrad);
		fprintf(io," sentl %d %ld %4.2f %4.2f %d %d %ld %4.2f\n",numsent,numwds,avw,avl,qcount,icount,numnonf,snonf);
		fprintf(io," l var %d %d %d %d %d",ml,lsum,mg,gsum,maxsent);
		fprintf(io," t var %d %d %d %d",simple,complex,compound,compdx);
		fprintf(io," verbs %d %d %d %d %d %d",tverbc,verbc,tobe,aux,infin,passive);
		fprintf(io," ty %d %d %d %d %d %d %d\n",prepc,conjc,adv,noun,adj,pron,nomin);
		fprintf(io," beg %d %d %d %d %d %d",beg[0],beg[7],beg[6],beg[3],beg[8],ii);
		fprintf(io," sbeg %d %d %d %d %d %d\n",beg[9],beg[4],beg[1]+beg[10]+beg[11],beg[13],beg[5],beg[14]);
		}
	}
#endif
	}
	return(1);
}
float
wperc(a)
{
	return((float)(a)*100/numwds);
}
float
sperc(a)
{
	return((float)(a)*100/numsent);
}
float
typersent(a)
{
return((float)(a)/numsent);
}
float
vperc(a)
{
	if(tverbc == 0)return(0);
	return((float)(a)*100/tverbc);
}
main(argc,argv)
char **argv;
{
	while(--argc > 0 && (++argv)[0][0] == '-' ){
		switch(argv[0][1]){
		case 'x': penflg = style = 1;
			continue;
		case 'y': pepflg = style = 1;
			continue;
		case 'z': pnpflg = style = 1;
			continue;
		case 'w': penpflg = style = 1;
			continue;
		case 'd': nosave = 0;
			continue;
		case 'S': style=1;
			continue;
		case 'p': pastyle=style=1;
			continue;
		case 's': style = sstyle = 1;
			sthresh = atoi(*(++argv));
			argc--;
			continue;
		case 'a': pstyle=style=1;
			continue;
		case 'e': estyle = style = 1;
			continue;
		case 'n': nstyle = style = 1;
			continue;
		case 'N': Nstyle = style = 1;
			continue;
		case 'l': style=lstyle=1;
			lthresh = atoi(*(++argv));
			argc--;
			continue;
		case 'r':
			style=rstyle=1;
			rthresh = atoi(*(++argv));
			argc--;
			continue;
		case 'P':
			part = 1;
			style = 0;
			continue;
		case 'b':		/* print bare bones info rje */
			barebones = 1;
			style = 0;
			continue;
		case 'T':		/*topic*/
			style = 0;
			topic = 1;
			continue;
		case 'L':		/*pass lineno info thru*/
			lineflg = 1;
			continue;
		default:
			fprintf(stderr,"unknown flag to part %s\n",*argv);
			exit(1);
		}
		argv++;
	}
#ifdef SNOM
	if(fopen(SNOM,"r") != NULL){
		deb = fopen(SNOM,"a");	/* SAVE NOM*/
	}
#else
	deb = NULL;
#endif
	yylex();
	if(nrofflg && sentno > 0){
		printf(".SL \"");
		for(i=0;i<sentno;i++)
			printf(" %d",leng[i]);
		printf("\"\n");
	}
}
