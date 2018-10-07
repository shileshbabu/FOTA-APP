/***************************************************************************************/

/***************************************************************************************/
/*	file: firmware_upgrade.c
 *
 * 	brief: Firmware upgrade app used to be download & upgrade new firmware.
 *
 *	auther: Shilesh Babu
 */
/****************************************************************************************/

/****************************************************************************************/
/*					Header Files														*/
/****************************************************************************************/
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <complex.h>
#include <ctype.h>
#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <inttypes.h>
#include <iso646.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <tgmath.h>
#include <pthread.h>
#include <time.h>
#include <uchar.h>
#include <wchar.h>
#include <wctype.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <curl/curl.h>

#include "firmware_upgrade_app.h"
#include "../../includes/common.h"
#include "../../includes/config.h"
#include "../../includes/version_control.h"

/******************************************************************************************/
/*					Macros																  */
/******************************************************************************************/
/******************************************************************************************/
/*					Enumerations														  */
/******************************************************************************************/

/* def: enum _PortNumbers
 * type: _PortNumbers
 * brief: Port numbers used for application
 */
typedef enum _PortNumbers
{
	APP_PORT = 0,
	PM_PORT = 1

}PortNumbers;

/******************************************************************************************/
/*					Global variable	& buffer 											  */
/******************************************************************************************/
/* var: firmware_upgrade_app_exit_flag
 * brief: Firmware upgrade application terminating/exiting reason
 */
char firmware_upgrade_app_exit_flag = 0;

/* var: firmware_version_flag
 * brief: Firmware version flag to read new version
 */
/* Current fw version */
int current_fw_version = 0;

/******************************************************************************************/
/*					Structures															  */
/******************************************************************************************/

/* structure:nodex_config_t
 * typedef: nodexConfig
 * brief: NodeX have device related configure
 */
//nodex_config_t nodexConfig;

/*******************************************************************************************/
/*					Function Defination													   */
/*******************************************************************************************/

/********************************************************************************************/
/*		fn: NDCommonReturnCodes Initialise_fw_upgrade_app(fw_upgrade_app_ctx *fw_upgrade_ctx)
 *
 *		brief: Initialize nodex firmware upgrade information
 *
 *		param: fw_upgrade_app_ctx fw_upgrade_ctx
 *
 *		return: NDCommonReturnCodes
 */
/*******************************************************************************************/
NDCommonReturnCodes Initialise_fw_upgrade_app(fw_upgrade_app_ctx *fw_upgrade_ctx);

/*******************************************************************************************/
/*		fn: NDCommonReturnCodes Deinitialise_fw_upgrade_app(fw_upgrade_app_ctx *fw_upgrade_ctx)
 *
 *		brief: De_initialize Nodex firmware upgrade
 *
 *		param: fw_upgrade_app_ctx *fw_upgrade_ctx
 *
 *  	return: NDCommonReturnCodes
 */
/*******************************************************************************************/
NDCommonReturnCodes Deinitialise_fw_upgrade_app(fw_upgrade_app_ctx *fw_upgrade_ctx);

/*******************************************************************************************/
/*		fn: static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream);
 *
 *		brief: download file from ftp server and store in given path
 *
 *		param: void *buffer, size_t size, size_t nmemb, void *stream
 *
 *  	return: size_t
 */
/*******************************************************************************************/
static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream);

/******************************************************************************************/
/*		fn: void Firmware_upgrade_thread(void *thread_arg)
 *
 *		brief: Thread to download & upgrade a new firmware
 *
 *		param: thread_arg - thread args
 *
 *		return: NULL
 */
/******************************************************************************************/
void *Firmware_upgrade_thread(void *thread_arg);

/*******************************************************************************************/
/*		fn: NDCommonReturnCodes  Create_devices_tmp_dir( char *temp_dir)
 *
 *		brief: created  temp directory
 *
 *  	param: char * temp_dir
 *
 *  	return: NDCommonReturnCodes
 */
/*******************************************************************************************/
NDCommonReturnCodes Create_devices_tmp_dir( char *temp_dir);

/*******************************************************************************************/
/*		fn: int Download_firmware_info(char *fw_download_path, char *fw_copy_dir)
 *
 *		brief: download & upgrade new firmware
 *
 *  	param: char *fw_download_path, char *fw_copy_dir
 *
 *  	return: int
 */
/*******************************************************************************************/
int Download_firmware_info(char *fw_download_path, char *fw_copy_dir);

/*******************************************************************************************/
/*		fn: int read_current_and_prev_fw_version(char *pre_fw_version_path, char *current_fw_version_path)
 *
 *		brief: Read all firmware version
 *
 *  	param: char *pre_fw_version_path, char *current_fw_version_path
 *
 *  	return: int
 */
/*******************************************************************************************/
int read_current_and_prev_fw_version(char *pre_fw_version_path, char *current_fw_version_path);

/********************************************************************************************/
/*					Function Declaration													*/
/********************************************************************************************/

/********************************************************************************************/
/*		fn: NDCommonReturnCodes Initialise_fw_upgrade_app(fw_upgrade_app_ctx *fw_upgrade_ctx)
 *
 *		brief: Initialize nodex firmware upgrade information
 *
 *		param: fw_upgrade_app_ctx fw_upgrade_ctx
 *
 *		return: NDCommonReturnCodes
 */
/*******************************************************************************************/
NDCommonReturnCodes Initialise_fw_upgrade_app(fw_upgrade_app_ctx *fw_upgrade_ctx){

#ifdef ENABLE_DEBUG_LOG
	FW_INFO(FW_UPGRADE_APP,"Initialize firmware upgrade application");
#endif

	do{
		/* Initialize shared memory */
		if ((Shared_memory_initialization(&(fw_upgrade_ctx->shm_data_trans_ctx.shm_data))) == -1){
#ifdef ENABLE_DEBUG_LOG
			FW_INFO(FW_UPGRADE_APP,"Fail to initializing shared memory in %s", __func__);
#endif
			ret_status = -1;
			break;
		}
		//Debug the sharm rd/wr issue
		sleep(10);


#if 0
		/* Initialize  semaphore  */
		if (0 != init_sem(&(fw_upgrade_ctx->firmware_upgrade_sem), 1)){
#ifdef ENABLE_DEBUG_LOG
			FW_ERROR(FW_UPGRADE_APP, "Fail to initialize  semaphore in %s", __func__);
#endif
			semaphore_ret_status = -1;
			break;
		}
#endif

		do {
			/* check either0net connectivity */
			if (system("ping -c2 google.com") == 0){
#ifdef ENABLE_DEBUG_LOG
				FW_ERROR(FW_UPGRADE_APP,  "-----------Internet Working -------------------");
#endif
				/* reset connected flag */
				net_connect_flag = 0;
				memset(&wait_time, 0x00, sizeof(wait_time));
				snprintf(fw_upgrade_ctx->firmware_upgrade_sem.sem_name, 256, "%s", NODEX_CONFIG_INFO);
				/* Initialize semaphore */
				if (-1 == open_sem(&(fw_upgrade_ctx->firmware_upgrade_sem))){
#ifdef ENABLE_DEBUG_LOG
					FW_ERROR(FW_UPGRADE_APP, "Fail to open  semaphore in %s", __func__);
#endif
					semaphore_ret_status = -1;
					break;
				}

				/* Get current epoch time */
				if(-1 == clock_gettime(CLOCK_REALTIME, &wait_time)){
					printf("Fail to get time at fun:%s line:%d\n", __func__, __LINE__);
					ret_status = -1;
					break;
				}
				else{
					/* setting timer in seconds. */
					wait_time.tv_sec += SEMAPHORE_TIMEOUT;
				}

		/* Create dir. to store device  fw  info */
		if (0 != (ret_status = Create_devices_tmp_dir(FW_DATA_PATH))){
#ifdef ENABLE_DEBUG_LOG
			FW_ERROR(FW_UPGRADE_APP,"Failed to created %s dir in func %s", FW_DATA_PATH, __func__);
#endif
			ret_status = -1;
			break;
		}
#endif

		/* Creating thread to download & upgrade firmware  */
		if (0 != pthread_create(&(fw_upgrade_ctx->firmware_upgrade.fw_upgrade_thread_id), NULL, &Firmware_upgrade_thread, fw_upgrade_ctx)){
#ifdef ENABLE_DEBUG_LOG
			FW_INFO(FW_UPGRADE_APP,"Failed to create firmware upgrade info thread = %s", __func__);
#endif
			ret_status = -1;
			/* Fail to create thread, reset thread init flag  */
			fw_upgrade_ctx->firmware_upgrade.fw_upgrade_thread_init = 0;
			break;
		}

		/* initialize firmware upgrade thread for further information */
		fw_upgrade_ctx->firmware_upgrade.fw_upgrade_thread_init = 1;
	}while(0);
	return ret_status;
}

/*******************************************************************************************/
/*		fn: NDCommonReturnCodes Deinitialise_fw_upgrade_app(fw_upgrade_app_ctx *fw_upgrade_ctx)
 *
 *		brief: De_initialize Nodex firmware upgrade
 *
 *		param: fw_upgrade_app_ctx *fw_upgrade_ctx
 *
 *  	return: NDCommonReturnCodes
 */
/*******************************************************************************************/
NDCommonReturnCodes Deinitialise_fw_upgrade_app(fw_upgrade_app_ctx *fw_upgrade_ctx)
{
	/* Func return status  */
	NDCommonReturnCodes ret_status = ND_SUCCESS;

	do{
#ifdef ENABLE_DEBUG_LOG
		FW_INFO(FW_UPGRADE_APP, "De-initialize firmware upgrade application");
#endif

		/* Joining firmware download & upgrade thread */
		pthread_join(fw_upgrade_ctx->firmware_upgrade.fw_upgrade_thread_id, NULL);
#ifdef ENABLE_DEBUG_LOG
		FW_ERROR(FW_UPGRADE_APP,"Join firmware upgrade thread successfully");
#endif

		/* Shared memory de_intialize */
		if ((Shared_memory_deinitialization(&(fw_upgrade_ctx->shm_data_trans_ctx.shm_data))) == -1){
#ifdef ENABLE_DEBUG_LOG
			FW_ERROR(FW_UPGRADE_APP,"Error while de_initializing shared memory");
#endif
			ret_status = -1;
			break;
		}
		/* De-initialize  semaphore */
		if (0 != deinit_sem(&(fw_upgrade_ctx->firmware_upgrade_sem))){
#ifdef ENABLE_DEBUG_LOG
			FW_ERROR(FW_UPGRADE_APP, "Fail to de-initialize semaphore");
#endif
			ret_status = -1;
			break;
		}
	}while(0);

	return ret_status;
}

/*******************************************************************************************/
/*		fn: NDCommonReturnCodes  Create_devices_tmp_dir( char *temp_dir)
 *
 *		brief: created  temp directory
 *
 *  	param: char * temp_dir
 *
 *  	return: NDCommonReturnCodes
 */
/*******************************************************************************************/
NDCommonReturnCodes Create_devices_tmp_dir(char * temp_dir)
{
	/* Func return status */
	char ret_status = 0;
	/* Dir. status */
	struct stat sb;

	/* Check dir. status */
	if (stat(temp_dir, &sb) == 0 && S_ISDIR(sb.st_mode)){
#ifdef ENABLE_DEBUG_LOG
		FW_ERROR(FW_UPGRADE_APP, "Directory is already present, no need to create new one %s", temp_dir);
#endif
	}
	else{
#ifdef ENABLE_DEBUG_LOG
		FW_ERROR(FW_UPGRADE_APP, "Directory is not present, creating  new one %s", temp_dir);
#endif
		/* Making temp. directory to store all connected device data */
		ret_status = mkdir(temp_dir, 0777);
		if(ret_status == -1){
#ifdef ENABLE_DEBUG_LOG
			FW_ERROR(FW_UPGRADE_APP, "Fail to create temp directory %s", temp_dir);
#endif
			ret_status = -1;;
		}
	}

	return ret_status;
}

/*******************************************************************************************/
/*		fn: read_current_and_prev_fw_version(char *pre_fw_version_path, char *current_fw_version_path)
 *
 *		brief: Read all firmware version
 *
 *  	param: char *pre_fw_version_path, char *current_fw_version_path
 *
 *  	return: int
 */
/*******************************************************************************************/
int read_current_and_prev_fw_version(char *pre_fw_version_path, char *current_fw_version_path)
{
	/* Return func status */
	int ret_status = 0;
	/* local buffer*/
	char local_buffer[50] = {0};
	/* Prev fw version */
	int prev_fw_version = 0;
	/* File pointer */
	FILE *fp = NULL;

	do {
		/* To check firmware path info */
		if ((pre_fw_version_path == NULL) || (current_fw_version_path == NULL)){
			break;
			ret_status = -1;
		}
		else{
			/* Check previous fw version file permission */
			if(access(pre_fw_version_path, F_OK) != -1 ){
				/* Check  file permission to read file name  */
				if (access(pre_fw_version_path, R_OK && W_OK && X_OK) == 0){
					//Reset fw  prev_fw_version
					prev_fw_version = 0;
					/* Open file to read/write data on file */
					fp = fopen(pre_fw_version_path, "r");
					if (fp == NULL){
#ifdef ENABLE_DEBUG_LOG
						FW_ERROR(FW_UPGRADE_APP, "Fail to open file in write mode %s in func %s ,line %d ", pre_fw_version_path, __func__, __LINE__);
#endif
						fclose(fp);
						ret_status = -1;
						break;
					}

					/* Write data to file from  buffer */
					/* Read from file */
					/* memset local buffer */
					memset(local_buffer, 0x00, sizeof(local_buffer));
					ret_status = fscanf(fp,"%s", local_buffer);
					prev_fw_version = atoi(local_buffer);
					printf("******** prev_fw_version Value ********* =%d \n", prev_fw_version);
					/* Close file ptr */
					if (fp != NULL)
						fclose(fp);
				}else {
					printf("File permission not valid  pre_fw_version_path = %s \n", pre_fw_version_path);
				}
			}else {
				printf("File does not exit = %s \n", pre_fw_version_path);
				//pre_fw_version_path
				/* Create file with name to store data */
				fp = fopen(pre_fw_version_path, "w");
				if (fp == NULL){
#ifdef ENABLE_DEBUG_LOG
					FW_ERROR(FW_UPGRADE_APP,  "Fail to create and open file %s  in func = %s", pre_fw_version_path , __func__);
#endif
					/* Close file pointer */
					fclose(fp);
					break;
				}
				if (fp != NULL)
					fclose(fp);
				/* Change file read/write permission */
				chmod(pre_fw_version_path, 0777);
				/* Default firmware version set */
				prev_fw_version = 0;
			}
			/* Check current fw version file permission */
			if(access(current_fw_version_path, F_OK) != -1 ){
				/* Check  file permission to read file name  */
				if (access(current_fw_version_path, R_OK && W_OK && X_OK) == 0){
					//Reset fw current_fw_version
					current_fw_version = 0;
					/* Open file to read/write data on file */
					fp = fopen(current_fw_version_path, "r");
					if (fp == NULL){
#ifdef ENABLE_DEBUG_LOG
						FW_ERROR(FW_UPGRADE_APP, "Fail to open file in write mode %s in func %s ,line %d ", current_fw_version_path, __func__, __LINE__);
#endif
						fclose(fp);
						ret_status = -1;
						break;
					}

					/* Write data to file from  buffer */
					/* Read from file */
					/* memset local buffer */
					memset(local_buffer, 0x00, sizeof(local_buffer));
					ret_status = fscanf(fp,"%s", local_buffer);
					current_fw_version = atoi(local_buffer);
					printf("******** current_fw_version Value ********* =%d \n", current_fw_version);
					/* Close file ptr */
					if (fp != NULL)
						fclose(fp);
					//TODO read current firmware version
					//TODO read prev firmware version
				}else {
					printf("File permission not valid current_fw_version_path = %s \n", current_fw_version_path);
#ifdef ENABLE_DEBUG_LOG
					FW_ERROR(FW_UPGRADE_APP, "File permission not valid current_fw_version_path = %s in function %s ,line %d ", current_fw_version_path, __func__, __LINE__);
#endif
				}
			}else {
				printf("*******************************************************current_fw_ver***********************File does not exit\n");
#ifdef ENABLE_DEBUG_LOG
				FW_ERROR(FW_UPGRADE_APP, "File %s  does not exit, in function %s ,line %d ",current_fw_version_path, __func__, __LINE__);
#endif
			}
			/* Compare previous & current firmware version */
			if (prev_fw_version == current_fw_version){
				ret_status = 0;
			}
			else {
				ret_status = 2;
			}
		}
	}while(0);
	return ret_status;
}

/*******************************************************************************************/
/*		fn: int Download_firmware_info(char *fw_download_path, char *fw_copy_dir)
 *
 *		brief: download & upgrade new firmware
 *
 *  	param: char *fw_download_path, char *fw_copy_dir
 *
 *  	return: int
 */
/*******************************************************************************************/
int Download_firmware_info(char *fw_download_path, char *fw_copy_dir)
{
	/* Return func status */
	int ret_status = 0;
	/* local buffer */
	char local_buffer[300] = {0};
	/* Curl command info */
	CURL *curl;
	CURLcode res;
	/* firmware path info */
	struct FtpFile ftpfile = {
			fw_copy_dir, /* name to store the file as if successful */
			NULL
	};

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if(curl) {
		}
	if(ftpfile.stream)
		fclose(ftpfile.stream); /* close the local file */

	curl_global_cleanup();

	return ret_status;
}

/*******************************************************************************************/
/*		fn: static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream);
 *
 *		brief: download file from ftp server and store in given path
 *
 *		param: void *buffer, size_t size, size_t nmemb, void *stream
 *
 *  	return: size_t
 */
/*******************************************************************************************/
static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct FtpFile *out = (struct FtpFile *)stream;
	if(out && !out->stream) {
		/* open file for writing */
		out->stream = fopen(out->filename, "wb");
		if(!out->stream)
			return -1; /* failure, can't open file to write */
	}
	return fwrite(buffer, size, nmemb, out->stream);
}

/*********************************************************************************/
/*		fn: void Firmware_upgrade_thread(void *thread_arg)
 *
 *		brief: Thread to upgrade new device firmware
 *
 *		param: thread_arg - thread args
 *
 *		return: NULL
 */
/*********************************************************************************/
void *Firmware_upgrade_thread(void *thread_arg)
{
	/* Return status */
	char ret_status = 0;
	/* local buffer */
	char local_buffer[200] = {0};
	/* Device dir info structure */
	struct stat stFileInfo;
	/* Time structure,calculate diagnostic data send interval */
	struct timespec t;
	/* Time structure,calculate diagnostic data send interval */
	struct timespec t1;
	/* firmware version down-load flag  */
	unsigned char fw_version_download_flag = 1;
	/* firmware down-load flag  */
	unsigned char fw_download_flag = 1;
	/* loop flag */
	unsigned char lp = 0;


	/* Check recv. thread arg */
	if (NULL == thread_arg){
#ifdef ENABLE_DEBUG_LOG
		FW_ERROR(FW_UPGRADE_APP, "Invalid thread argument received in %s function", __func__);
#endif
		return NULL;
	}
#ifdef ENABLE_DEBUG_LOG
	FW_INFO(FW_UPGRADE_APP, "%s created", __func__);
#endif

	/* Copy thread arg into firmware app structure */
	fw_upgrade_app_ctx *fw_upgrade_ctx = (fw_upgrade_app_ctx *)thread_arg;

	do{
		/* Infinite loop till app & thread alive */
		while((0 == firmware_upgrade_app_exit_flag) && (fw_upgrade_ctx->firmware_upgrade.fw_upgrade_thread_init == 1)){
			/* Down-load firmware version info */
			ret_status = Download_firmware_info(FW_VER_INFO_PATH, KEEP_FW_VER_PATH);
			if (ret_status != 0){
				/* Fail to down-load fw version, Reset fw version flag */
				fw_version_download_flag = 1;
#ifdef ENABLE_DEBUG_LOG
				FW_ERROR(FW_UPGRADE_APP, "Fail to down-load firmware version  in function %s ,line %d ", __func__, __LINE__);
#endif
			}
			else{
				/* Down-load fw version successfully */
				fw_version_download_flag = 0;
				/* Compare fw version */
				ret_status = read_current_and_prev_fw_version(DEVICE_FW_INFO, KEEP_FW_VER_PATH);
				if (ret_status == -1){
					printf("fail to check fw version \n");
#ifdef ENABLE_DEBUG_LOG
					FW_ERROR(FW_UPGRADE_APP, "Fail to read current & previous fw version in function %s ,line %d ", __func__, __LINE__);
#endif
				}
				else if (ret_status == 2){ /* New Device firmware present down load it */
					while ((fw_download_flag) && (0 == firmware_upgrade_app_exit_flag)){
						/* flush local buffer */
						memset(local_buffer, 0x00, sizeof(local_buffer));
						/* Check fw release */
						if (current_fw_version < 10){
							snprintf(local_buffer, sizeof(local_buffer), "%s_000%d.tar.gz", NEW_FW_PATH, current_fw_version);
						}
						else {
							snprintf(local_buffer, sizeof(local_buffer), "%s_00%d.tar.gz", NEW_FW_PATH, current_fw_version);
						}
						//DEBUG
						//printf("Debug local_buffer = %s \n", local_buffer);
						//printf("Debug KEEP_FW_PATH = %s \n", KEEP_FW_PATH);
						//start down loading
						/* Down load firmware version info */
						ret_status = Download_firmware_info(local_buffer, KEEP_FW_PATH);
						if (ret_status != 0){
#ifdef ENABLE_DEBUG_LOG
							FW_ERROR(FW_UPGRADE_APP, "Fail to down-load New device firmware  in function %s ,line %d ", __func__, __LINE__);
#endif
							/* Fail to down-load new firmware  */
							fw_download_flag = 1;
							for (lp = 0; lp < 120; lp++){
								if (0 == firmware_upgrade_app_exit_flag)
									sleep(1);
							}
						}
						else {
							fw_download_flag = 0;
							/* Untar down-loaded firmware */
							/* memset local buffer */
							memset(local_buffer, 0x00, sizeof(local_buffer));
							snprintf(local_buffer, sizeof(local_buffer), "sudo tar -xvzf %s -C %s", KEEP_FW_PATH, FW_PATH);
							if (system(local_buffer) < 0){
								printf("fail to untar files \n");
							}

							/* Stop all running device services */
							/* memset local buffer */
							memset(local_buffer, 0x00, sizeof(local_buffer));
							/* Stop all running services */
							snprintf(local_buffer, sizeof(local_buffer), "sudo %s", STOP_ALL_SERVICES);
							if (system(local_buffer) < 0){
								printf("fail to stop all  running services\n");
							}

							/* New firmware going to upgrade on device */
							/* Check fw version  */
							if (current_fw_version < 10){
								/* memset local buffer */
								memset(local_buffer, 0x00, sizeof(local_buffer));
								/* Stop all running services */
								snprintf(local_buffer, sizeof(local_buffer), "sudo mv %s%s_000%d/* %s", FW_PATH, "fw_release", current_fw_version, "/usr/local/bin/");
								if (system(local_buffer) < 0){
									printf("fail to copy firmware bun services\n");
								}
							}
							else{
								/* New firmware going to upgrade on device */
								/* memset local buffer */
								memset(local_buffer, 0x00, sizeof(local_buffer));
								/* Stop all running services */
								snprintf(local_buffer, sizeof(local_buffer), "sudo mv %s%s_00%d/* %s", FW_PATH, "fw_release", current_fw_version, "/usr/local/bin/");
								if (system(local_buffer) < 0){
									printf("fail to copy firmware bun services\n");
								}
							}
#if 0
							//TODO
							/* delete all storage firmware
							/* memset local buffer */
							memset(local_buffer, 0x00, sizeof(local_buffer));
							/* Stop all running services */
							snprintf(local_buffer, sizeof(local_buffer), "sudo rm -rf %s/*", FW_PATH);
							if (system(local_buffer) < 0){
								printf("fail to stop all  running services\n");
							}
#endif
							/* Firmware upgrade successfully, now write current version as previous fw version */
							/* Firmware current version differ from previous version */
							/* memset local buffer */
							memset(local_buffer, 0x00, sizeof(local_buffer));
							/* Stop all running services */
							snprintf(local_buffer, sizeof(local_buffer), "sudo cp -r %s %s", KEEP_FW_VER_PATH, DEVICE_FW_INFO);
							if (system(local_buffer) < 0){
								printf("fail to upgrade new firmware version\n");
							}
#ifdef ENABLE_DEBUG_LOG
							FW_ERROR(FW_UPGRADE_APP, "Device going to delete down-loaded new firmware in %s function", __func__);
#endif
							/* Sleep for one second  & sync the process */
							sleep(1);
							sync();
							/* Delet down-loaded Firmware current version */
							/* memset local buffer */
							memset(local_buffer, 0x00, sizeof(local_buffer));
							/* Stop all running services */

							if (system(local_buffer) < 0){
								printf("fail to remove down-loaded firmware info \n");
							}
#ifdef ENABLE_DEBUG_LOG
							FW_ERROR(FW_UPGRADE_APP, "Device going to reboot because of new firmware upgrade in %s function", __func__);
#endif
							/* Close firmware upgrade app flag reset */
							firmware_upgrade_app_exit_flag = 1;
						}
					}
				}
				else {
					/* No need to upgrade firmware */
					/* Reset all fw down-load & upgrade flag */
#ifdef ENABLE_DEBUG_LOG
					FW_ERROR(FW_UPGRADE_APP, "Device firmware up-to-date no need to down-load new firmware,  %s function", __func__);
#endif
					fw_download_flag = 0;
					fw_version_download_flag = 0;
					firmware_upgrade_app_exit_flag = 1;
				}
			}
		}

		FW_ERROR(FW_UPGRADE_APP, "%s thread exited", __func__);
		/*Reset firmware  upgrade flag*/
		fw_upgrade_ctx->firmware_upgrade.fw_upgrade_thread_init = 0;
		pthread_exit(NULL);
	}while(0);
	return NULL;
}
