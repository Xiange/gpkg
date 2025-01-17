/*
	xgfileinfo: file record tool for Xiange Linux package management system
	file_info.c: main program of xgfileinfo

	by Pinkme005 @ LinuxSir (First version)
	Modified by Zhang Lihui <swordhuihui@gmail.com>
*/
	
#include "stdio.h"
#include "string.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "libcsv.h"
#include "finfo.h"
#include "bprogress.h"

extern  int MDFile(const char *pName, char* pMd5Sum, int iBufLen);

#define MAX_NAME_LEN    (4096)
#define FILE_MODE_MASK 	0x1FF 
#define TIME_MODIFY_BUF	26
#define	MD5		5
#define	SHA256		256

int get_info(char* csFileName);

static long long f_llFileSize=0;
static long 	f_lDiskUsage=0;
static int	f_iDCount=0;
static int	f_iFCount=0;
static int	f_iSCount=0;
static char f_NewBase[1024]="";
const char *applet_name;

/*
	Arguments 
	   -cm		check the file using md5 module
	   -cs		check the file using shax module (reserved space)
	   -f		filename list (optional,for losts of files)
	   filename 	single file (default use)
 */   

//from busybox.
int mv_main(int argc, char **argv);
int rm_main(int argc, char **argv);
int rmdir_main(int argc, char **argv);

//read file list from stdin, normally it's output of find progrem.
//dump file information to stdout, such as path, type, md5sum, time..
static int do_file_info(void)
{
	char    *pRet=NULL;
	char    csName[MAX_NAME_LEN];
	int     count=0;
	int     len,i,verify_mode;


	while(1)
	{
		pRet=fgets(csName, MAX_NAME_LEN, stdin);
		if(NULL == pRet) break;

		//remove \n
		len=strlen(csName);
		if(csName[len-1] == '\n') csName[len-1]=0;

		//remov "."
		if(strcmp(csName, ".") == 0) continue;

		//to stdout.
		get_info(csName);
		count++;
	}

	//Show info. I,Fcount,Dcount,Scount,SpaceUsage,ReadSize
	printf("I,%d,%d,%d,%ld,%d\n", f_iFCount, f_iDCount, f_iSCount,
		f_lDiskUsage/2, (int)((f_llFileSize-1)/1024+1));

	return 0;
}

static int print_usage(void)
{
	printf("\nxgfileinfo v1.0 usage:\n");
	printf("\t--help		: this screen.\n");
	printf("\t-u file		: rolling update according to file.\n");
	printf("\t-d file		: Remove package according to file.\n");
	printf("\t-c file		: check package according to file.\n");
	printf("\t-mv src dst	: Move src to dst.\n");
	printf("\t-rm file		: delete file.\n");
	printf("\t-rmdir dir	: delete dir.\n");
	printf("\t-root dir		: set new root instead of /\n");
	printf("\n-----------------------\n");
}


static int show_progress(int iFilePass, int iFileErr)
{
	char csInfo[80];	

	sprintf(csInfo, "%d records PASS, %d records FAIL", iFilePass, iFileErr);
	bp_clear();
	bp_printf(csInfo);
}

static f_iPassCnt=0;
static f_iFailCnt=0;

static void show_error(const char* fname, int iResult, FINFO* pReal,
	FINFO* pRecord)
{
	bp_clear();
	fprintf(stderr, "\n");
	finfo_showResult(iResult, fname, pReal, pRecord);
	fprintf(stderr, "\n");
}

static int csvCheckFile(int iIndex, int iLineNo, char* pLine)
{
	//return 0 means OK, other means failed and program shoudl be quit
	int iRet=0;
	FINFO real, rec;

	const char* pCsvSeg[16];
	int iSegCnt;
	int iResult;
	char csNewName[2048];

	//break line to segs.
	libcsv_getseg(pLine, pCsvSeg, 16);

	strncpy(csNewName, f_NewBase, 1024);
	strcat(csNewName, pCsvSeg[1]);


	//check version.
	switch(pLine[0])
	{
		case 'D':
			finfo_get(csNewName, &real);
			finfo_getFromCSVSeg(pCsvSeg, 16, &rec);
			iResult=finfo_cmp(&real, &rec, FINFO_CKMASK_DIR);

			if(iResult == 0)
				//pass.
				f_iPassCnt++;
			
			else
			{
				f_iFailCnt++;
				show_error(csNewName, iResult, &real, &rec);
			}

			show_progress(f_iPassCnt, f_iFailCnt);
		

			break;

		case 'F':
			//file.
			finfo_get(csNewName, &real);
			finfo_getFromCSVSeg(pCsvSeg, 16, &rec);
			iResult=finfo_cmp(&real, &rec, FINFO_CKMASK_FILE);

			if(iResult == 0)
				//pass.
				f_iPassCnt++;
			
			else
			{
				f_iFailCnt++;
				show_error(csNewName, iResult, &real, &rec);
			}

			show_progress(f_iPassCnt, f_iFailCnt);
		

			break;

		case 'O':
			//other file, char/block/fifo
			finfo_get(csNewName, &real);
			finfo_getFromCSVSeg(pCsvSeg, 16, &rec);
			iResult=finfo_cmp(&real, &rec, FINFO_CKMASK_DIR);

			if(iResult == 0)
				//pass.
				f_iPassCnt++;
			
			else
			{
				f_iFailCnt++;
				show_error(csNewName, iResult, &real, &rec);
			}

			show_progress(f_iPassCnt, f_iFailCnt);
		

			break;



		case 'S':
			//Symbol link.
			finfo_get(csNewName, &real);
			finfo_getFromCSVSeg(pCsvSeg, 16, &rec);
			iResult=finfo_cmp(&real, &rec, FINFO_CKMASK_LINK);

			if(iResult == 0)
				//pass.
				f_iPassCnt++;
			
			else
			{
				f_iFailCnt++;
				show_error(csNewName, iResult, &real, &rec);
			}

			show_progress(f_iPassCnt, f_iFailCnt);

		
			break;

		case 'I':
			//printf("<I> %s\n", pCsvSeg[1]);
			//information.
			break;

		default:
			bp_clear();
			printf("<U> %s\n\n", pLine);
			break;
	}

	return iRet;
}

int main(argc,argv)
int argc;
char **argv;	
{
	applet_name = argv[0];
	int i;

	if(argc == 1)
	{
		//No argument.
		return do_file_info();
	}

	//check parameters
	for(i=1; i<argc; i++)
	{
		if(strcmp(argv[i], "-root") == 0)
		{
			strncpy(f_NewBase, argv[i+1], 1024);
			//printf("NewBase set to %s\n", f_NewBase);
		}
	}

	for(i=1; i<argc; i++)
	{
		//check parameter: help
		if(strcmp(argv[i], "--help") == 0)
		{
			return print_usage();
		}

		//check parameter: mv
		if(strcmp(argv[i], "-mv") == 0)
		{
			char *newgv[5];

			newgv[0]="mv";
			newgv[1]="-f";
			newgv[2]=argv[i+1];
			newgv[3]=argv[i+2];
			newgv[4]=NULL;

			//call busybox mv
			return mv_main(4, newgv);
		}

		//check parameter: rm
		if(strcmp(argv[i], "-rm") == 0)
		{
			char *newgv[4];

			newgv[0]="rm";
			//newgv[1]="-r";
			newgv[1]=argv[i+1];
			newgv[2]=NULL;

			//call busybox mv
			return rm_main(3, newgv);
		}

		//check parameter: rmdir
		if(strcmp(argv[i], "-rmdir") == 0)
		{
			char *newgv[4];

			newgv[0]="rmdir";
			//newgv[1]="-r";
			newgv[1]=argv[i+1];
			newgv[2]=NULL;

			//call busybox mv
			return rmdir_main(3, newgv);
		}

		//check parameter: -c
		if(strcmp(argv[i], "-c") == 0)
		{
			const char* filename=argv[i+1];

			bp_init();

			//parse csv file.
			libcsv_init();

			libcsv_parse(filename, csvCheckFile);

			printf("\n\n");
		}
	}



	return 0;
}

//routine for get file information, such as file tye, file Last Modify Time,
//file mode, file size, file md5 checksum, and output to stdout.
// file type:  'D' Directory. 'S' Symbol link. 'F' -- Normal file.
// if file type is 'F', then output name, mode, LMT, size,  md5checksum.
// if 'D', output name, mode, LMT
// if 'S', output name, mode, LMT, link destination.
int get_info(char* csFileName)
{
	struct stat 	info;
	int 		mask_test;
	long int	t_modify;
    long int	iTestSize;
    char            csTestSum[128];
	char		*pOutName;

    //type 'F' , name, mode, LMT, size, cksum
	//printf("file name is :%s\n",csFileName);
	if(lstat(csFileName,&info)!=0)
	{
		//printf("File is not exist!\n");
		return (1);
	}

	//remove leading . for output.
	pOutName=(csFileName[0] == '.' ? csFileName+1 : csFileName);

	iTestSize=info.st_size;
	t_modify=info.st_mtime;
	mask_test=info.st_mode;

	//csLMT=ctime(&t_modify);
	//get type.
	if(S_ISLNK(info.st_mode))
	{
		//Links. output "S,Name,Size,Mask,LMT
		printf("S,%s,%d,%04o,%d,%d,%d\n", pOutName, iTestSize,
			mask_test&FILE_MODE_MASK, info.st_uid, info.st_gid,
			t_modify);

		f_iSCount++;
		f_lDiskUsage+=info.st_blocks;
			
	}
	else if(S_ISDIR(info.st_mode))
	{
		//Dir, output "D,Name,Size,Mask,LMT
		printf("D,%s,%d,%04o,%d,%d,%d\n", pOutName, iTestSize,
			mask_test&FILE_MODE_MASK, info.st_uid, info.st_gid,
			t_modify);

		f_iDCount++;
		f_lDiskUsage+=info.st_blocks;
	}
	else if(S_ISBLK(info.st_mode) || S_ISCHR(info.st_mode) || 
			S_ISFIFO(info.st_mode))
	{
		//others file, output "F,Name,Size,Mask,uid, gid,LMT,Md5Sum
		printf("O,%s,%d,%04o,%d,%d,%d,%04x\n", pOutName, iTestSize,
			mask_test&FILE_MODE_MASK, info.st_uid, info.st_gid,
			t_modify, info.st_mode);

		f_iFCount++;
		f_lDiskUsage+=info.st_blocks;
		f_llFileSize+=iTestSize;

	}
	else
	{
		//get md5 sum
		MDFile(csFileName, csTestSum, 128);

		//file, output "F,Name,Size,Mask,uid, gid,LMT,Md5Sum
		printf("F,%s,%d,%04o,%d,%d,%d,%s\n", pOutName, iTestSize,
			mask_test&FILE_MODE_MASK, info.st_uid, info.st_gid,
			t_modify, csTestSum);

		f_iFCount++;
		f_lDiskUsage+=info.st_blocks;
		f_llFileSize+=iTestSize;
	}

	return 0;
}

void bb_show_usage(void)
{   
	//fputs(APPLET_full_usage "\n", stdout);
	exit(EXIT_FAILURE);
}

