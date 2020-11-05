/****************************************************************************
 *   ProjectName : 
 *
 *   COMPONENT   : ftp.c
 *
 *    Author     : Gobikrishnan Ramamoorthi
 *
 *    DATE       : 
 *
 *   Description : Delete a file to the cloud server
 *
*******************************************************************************/

/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

//common return values
#define G_PASS 0
#define G_FAIL -1

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
* Global variable
******************************************************************************/
Char caFtpHost[MAXSTR]    = {'\0'};
Char caFtpUser[MAXSTR]    = {'\0'};
Char caFtpPwd[MAXSTR]    = {'\0'};
Char caCamera[MAXSTR]    = {'\0'};
Char caFile[MAXSTR]    = {'\0'};
Char caFtpUrl[MAXSTR]    = {'\0'};

/****************************************************************************
 *
 * NAME          :  main
 *
 * PARAMETERS    :  void
 *
 * RETURN VALUES :  failure->G_FAIL
 *                  success->G_PASS
 *
 * DESCRIPTION   :  Main Function of init the parameter and delete the partial 
 *                  file from the ftp server 
 *
****************************************************************************/
int main(int argc, char **argv)
{
    CURL *curl;
    CURLcode res;

    if(argc != 6)
    {
         printf("Usage : ./ftp_delete <ftp_host> <user> <psw> <camera> <file_name>\n");
         return -1;
    }

    strcpy(caFtpHost, argv[1]);
    strcpy(caFtpUser, argv[2]);
    strcpy(caFtpPwd, argv[3]);
    strcpy(caCamera, argv[4]);
    strcpy(caFile, argv[5]);

    /* Get the ftp parameter from shared memory*/
    iInitFTP();

    sprintf(caFtpUrl, "ftp://%s/%s/", caFtpHost, caCamera );
    printf("caFtpUrl = %s\n", caFtpUrl);
  
    Char buf_1 [MAXSTR] = {'\0'} ;
    sprintf(buf_1, "DELE %s", caFile);
    curl = curl_easy_init();
    printf("curl init success\n");
    if(curl) 
    {
        //headerlist = curl_slist_append(headerlist, buf_1);
        curl_easy_setopt(curl, CURLOPT_URL, caFtpUrl);
        curl_easy_setopt(curl, CURLOPT_USERNAME, caFtpUser);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, caFtpPwd);
        curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, buf_1);

        /* pass in that last of FTP commands to run after the transfer */ 
        printf("After curl_easy_setopt\n");
        res = curl_easy_perform(curl);
        if(CURLE_OK != res) 
        {
            /* we failed */
            fprintf(stderr, "curl replied: %d\n", res);
        }
    }
    /* clean up the FTP commands list */ 

    curl_easy_cleanup(curl);
    return 0;
} 

/****************************************************************************
 *
 * NAME          :  iInitFTP
 *
 * PARAMETERS    :  int
 *
 * RETURN VALUES :  failure->G_FAIL
 *                  success->G_PASS
 *
 * DESCRIPTION   :  Initialize the ftp parameter
 *
****************************************************************************/
int iInitFTP()
{

    //strcpy(acFTPfolder,FTP_FOLDER); //hardcoded
    sprintf(caCamera,"%s", "/htdocs/Backup/");
    strcpy(caFtpHost,"ftpupload.net");
    strcpy(caFtpPwd,"EJ7Y007ruLZrz")
    strcpy(caFtpUser,"epiz_24721629");

    return 0;

}
