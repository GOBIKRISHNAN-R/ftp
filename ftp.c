/****************************************************************************
 *   ProjectName : 
 *
 *   COMPONENT   : ftp.c
 *
 *    Author     : Gobikrishnan Ramamoorthi
 *                 
 *    DATE       : 
 *
 *   Description : upload a file to the cloud server
 *
*******************************************************************************/

/******************************************************************************
* Include Files
******************************************************************************/
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>


/******************************************************************************
* Macro Definition
******************************************************************************/
#define FTP_FOLDER  "camera1"
#define E_SUCCESS 0
#define E_FAILURE 1
#define FTP_PATH "./files"
#define REC_MNT_PATH "./videos/"

//common return values
#define G_PASS 0
#define G_FAIL -1

//shared mem key
#define SHARED_KEY 1234

//string sizes
#define STRG_64  64
#define STRG_128 128
#define STRG_256 256
#define STRG_512 512
#define MAXSTR   1024
#define MAX_ARR  10

#define Void    void
#define Int     int
#define s32     int
#define Char    char
/******************************************************************************
* Global variables
******************************************************************************/
char acFTPuser[STRG_64];
char acFTPpass[STRG_64];
char acFTPserver[STRG_64];
char acFTPfolder[STRG_64];
int iFTPflag=1;

int iFtp_backup_flag = 0;
int iCount = 0;
int iAppState = 0;
int iFileComplete = 0;
FILE *fd;

/******************************************************************************
* Function Prototypes
******************************************************************************/
int uploadfile(char *pcFilename,char *pcFile);

Void ftp_ipc_handler( Void *vpMsg , Int MsgLen );

/* This function called, when signal interrupt comes */
Void sighandler(Int iSignum);
Void sighandlerINT(Int iSignum);
void rm_space(char * string);

int main(void)
{
    int FTP_FLAG = 0;
    int iftp_stable = 0;
    char cTime[STRG_512];
    char caHealthMonitor[STRG_512];
    char acBuf[STRG_512];
    char cmd[STRG_512];
    int iDate = 0;
    FILE *fp, *fc;
    time_t result;

    result = time(NULL);
    sprintf(cTime, "%s", asctime(localtime(&result)));
    cTime[ strlen(cTime)-1] = '\0' ;

    time_t T= time(NULL);
    struct  tm tm = *localtime(&T);
    iDate = tm.tm_mday;

    /*  Interrupt signal handler  */
    signal(SIGINT, sighandler);
    /*  Terminate signal handler  */
    signal(SIGTERM, sighandler);

    printf("main called from ftp application\n");
    //read server details
    if ( iInitFTP() != E_SUCCESS )
    {
    	printf("iInitFTP() Filed\n");
        return E_FAILURE; 
    }

    /* Check ftp partitial file is availvale or not */
    if ( iCheckftpfiles() != E_SUCCESS )
    {
        return E_FAILURE;
    }

    if(iFTPflag == 1 )
    {
        //while(iAppState != 1)
        while(1)
        {
            if ( FTP_FLAG == 3 )
            {
                result = time(NULL);
                sprintf(cTime, "%s", asctime(localtime(&result)));
                cTime[ strlen(cTime)-1] = '\0' ;

                time_t T= time(NULL);
                struct  tm tm = *localtime(&T);
                iDate = tm.tm_mday;
                printf("EXitting....\n");
                exit(0);
            }
            //upload files
            if( iUploadrecordedfiles() != E_SUCCESS)
            {         
                FTP_FLAG = FTP_FLAG + 1;
                printf("Something happened... [%d] Times...\n", FTP_FLAG);
            }
            else
            {
                FTP_FLAG = 0;
                printf("working properly...\n");
            }
            sleep(1);
        }
    }
    else
    {
         printf("FTP push not enabled in configuration\n");
    }

    printf( "FTP properly closed!!!\n");
    
    sleep(1);
    return 0;
}

int isExists(char *caFile)
{
    struct stat sb;
	
    if (stat(caFile, &sb) == 0)
    {
        printf("File exists\n");
        return G_PASS;
    }
    else
    {
        printf("File not exists\n");
        return G_FAIL;
    }

}

int iUploadrecordedfiles()
{
    FILE *fb=NULL;
    char acPath[STRG_128]={'\0'};
    char acEntry[STRG_128]={'\0'};
    char acFile[STRG_256]={'\0'};
    char caMsg[STRG_256]={'\0'};

    sprintf(acPath,"ls %s",REC_MNT_PATH);

    //opening firmware path
    fb = popen(acPath,"r");
    if(fb == NULL)
    {
        printf("path error popen\n");
        return E_FAILURE;
    }
    fgets(acEntry ,sizeof(acEntry)-1 ,fb);
    pclose(fb);
    printf("read entry=(%s)\n",acEntry);

    if(acEntry[0] != '\0')
    {
        sscanf(acEntry,"%[^ \n]",acEntry);
        printf("acFilename=%s \n",acEntry);
    }
    else
    {
	printf("Not having files in %s \n",FTP_PATH);
    	return E_FAILURE;
    }

    if(strstr(acEntry,".mp4") || strstr(acEntry,".jpg"))
    {
        sprintf(acFile,"%s%s",REC_MNT_PATH,acEntry);
    }
    else
    {
        sprintf(acFile,"%s%s",FTP_PATH,acEntry); //DIVYA
    }
   
    printf("acFile =(%s) File=(%s)\n",acFile,acEntry);

    if(uploadfile(acFile,acEntry) == 0)
    {
        printf("uploading file success\n");
    }
    else 
    {
        printf("Error in uploading file\n");
        return E_FAILURE;
    }
    return E_SUCCESS;
}

int iInitFTP()
{
    //strcpy(acFTPfolder,FTP_FOLDER); //hardcoded
    sprintf(acFTPfolder,"%s", "htdocs/Backup/"); 
    strcpy(acFTPserver, "ftpupload.net");
    strcpy(acFTPpass,"EJ7Y007ruLZrz");
    strcpy(acFTPuser,"epiz_24721629");

    return E_SUCCESS;
}

int uploadfile(char *pcFilename,char *pcFile)
{
    CURL *curl;
    CURLcode res;
    struct stat file_info;
    int ret_size = 0;
    int MB = 1048576;
    float size_of_file = 0.0 ;
    double speed_upload, total_time;
    char acCurlurl[STRG_512];
    char acBuf[STRG_512];
    char cTime[STRG_512];
    char caHealthMonitor[STRG_512];
    int iDate = 0;
    time_t result;

    result = time(NULL);
    sprintf(cTime, "%s", asctime(localtime(&result)));
    cTime[ strlen(cTime)-1] = '\0' ;

    time_t T= time(NULL);
    struct  tm tm = *localtime(&T);
    iDate = tm.tm_mday;

    sprintf(acCurlurl,"ftp://%s/%s/%s",acFTPserver,acFTPfolder,pcFile);

    printf("%s\n",acCurlurl);

    fd = fopen(pcFilename,"rb"); /* open file to upload */

    /* to get the file size */
    if(fstat(fileno(fd), &file_info) != 0)
    {
        printf("Not getting file size\n");
        return E_FAILURE; /* can't continue */
    }
        
    ret_size = file_info.st_size;
    
    if (ret_size > 0)
    {
        size_of_file = (float) ret_size / (float) MB;
        printf("%s having size of %f MB\n", pcFile, size_of_file);
    }

    if ( ret_size > 1000)
    {
        printf("%s having %d bytes.. So it is Uploading to FTP Server \n", pcFile, ret_size);
        curl = curl_easy_init();
        if(curl) 
        {
            /* upload to this place */
            curl_easy_setopt(curl, CURLOPT_URL,acCurlurl);

            curl_easy_setopt(curl, CURLOPT_USERNAME,acFTPuser);
            curl_easy_setopt(curl, CURLOPT_PASSWORD,acFTPpass);
            curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
            /* tell it to "upload" to the URL */
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    
            /* set where to read from (on Windows you need to use READFUNCTION too) */
            curl_easy_setopt(curl, CURLOPT_READDATA, fd);

            /* set the timeout */
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 180L);
   
            /* and give the size of the upload (optional) */
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)file_info.st_size);
   
            /* enable verbose for easier tracing */
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  
            res = curl_easy_perform(curl);
            /* Check for errors */
            if(res != CURLE_OK) 
            {
                fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
                return E_FAILURE;
            }
            else 
            {
                /* now extract transfer info */
                curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed_upload);
                curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
    
                fprintf(stderr, "Speed: %.3f bytes/sec during %.3f seconds\n",speed_upload, total_time);
            }
            /* always cleanup */
            curl_easy_cleanup(curl);
        }
        fclose(fd);

        /* Remove actual file from mount partition */
        memset(acBuf, '\0', sizeof(acBuf));
        sprintf(acBuf,"rm %s%s", REC_MNT_PATH, pcFile);
        printf("%s\n",acBuf);
        printf("file to be removed=(%s)\n",acBuf);
 
        if(system(acBuf) != G_PASS)
        {
            printf("Error in deleting uploaded file\n");
            return E_FAILURE;
        }

        iCount++;

        iFileComplete = 1;
    }
    else
    {
        printf("%s Doesn't Having buffers... \n", pcFile);
        sprintf(acBuf,"unlink %s%s",FTP_PATH,pcFile);
        printf("%s\n",acBuf);
        printf("file to be removed=(%s)\n",acBuf);
   
        if(system(acBuf) != G_PASS)
        {
            printf("Error in deleting uploaded file\n");
            return E_FAILURE;
        }
 
        /* Remove actual file from mount partition */
        memset(acBuf, '\0', sizeof(acBuf));
        sprintf(acBuf,"rm %s%s", REC_MNT_PATH, pcFile);
        printf("%s\n",acBuf);
        printf("file to be removed=(%s)\n",acBuf);
 
        if(system(acBuf) != G_PASS)
        {
            printf("Error in deleting uploaded file\n");
            return E_FAILURE;
        }
    }
    return E_SUCCESS;
}

Int iCheckftpfiles()
{
    FILE *fb=NULL;
    Char acPath[MAXSTR]={'\0'};
    Char acEntry[MAXSTR]={'\0'};
    Char acDeleteftpfile[MAXSTR]={'\0'};
    Int iRet = 0;

    sprintf(acPath,"ls %s",FTP_PATH);

    //opening firmware path
    fb = popen(acPath,"r");
    if(fb == NULL)
    {
        printf("path error popen\n");
        return E_FAILURE;
    }
    fgets(acEntry ,sizeof(acEntry)-1 ,fb);
    pclose(fb);
    printf("1. read entry=(%s)\n",acEntry);

    return E_SUCCESS;
}

/****************************************************************************
 *
 * NAME          :  sighandler
 *
 * PARAMETERS    :  Void
 *
 * RETURN VALUES :  -
 *
 * DESCRIPTION   :  This function called, when signal interrupt comes
 *
****************************************************************************/
Void sighandler(Int iSignum)
{
    char cTime[STRG_512];
    char caHealthMonitor[STRG_512];
    char acBuf[STRG_512];
    char cmd[STRG_512];
    int iDate = 0;
    FILE *fp, *fc;
    time_t result;

    result = time(NULL);
    sprintf(cTime, "%s", asctime(localtime(&result)));
    cTime[ strlen(cTime)-1] = '\0' ;

    time_t T= time(NULL);
    struct  tm tm = *localtime(&T);
    iDate = tm.tm_mday;
    iAppState = 1;
    sleep(1);
    exit(0);
}

/****************************************************************************
 *
 * NAME          :  remove_space
 *
 * PARAMETERS    :  Void
 *
 * RETURN VALUES :  -
 *
 * DESCRIPTION   :  remove the space for given string
 *
****************************************************************************/
void rm_space(char * string)
{
    int ilastIndex = 0;
    int iIndex = 0;
    while(string[iIndex] != '\0')
    {
        if(string[iIndex] != ' ' && string[iIndex] != '\t' && string[iIndex] != '\n')
        {
            ilastIndex = iIndex;
        }
        iIndex++;
    }
    string[ilastIndex + 1] = '\0';
}
