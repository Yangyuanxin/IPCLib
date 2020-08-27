/**************************************************************************
 * 	FileName:		osd.c
 *	Description:	OSD Process(��Ļ����)
 *	Copyright(C):	2006-2008 HHDigital Inc.
 *	Version:		V 1.0
 *	Author:			ChenYG
 *	Created:		2006-08-06
 *	Updated:
 *
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

//#include "global_config.h"
#include "hh_osd.h"
#include "hh_osd_api.h"
//#include "toolbox.h"
//#include "syslog.h"
#include <sys/socket.h> 
#include <sys/un.h>
#include <stdint.h>  
#define UNIX_DOMAIN "/tmp/UNIX.domain"


HH_VIDEO_RESOLUTION ipc_conf;

int UART_TEMP_FLAG;

HH_OSD_LOGO		*pGOsdLog[HH_MAX_OSD_NUM];	//={0}
VIDEO_OSD_STATUS *pvideo_osd_status;
struct myosd_text
{
    char  Head;
    char  token[64];
    char  VideoSourceConfigurationToken[64];	
    char  PosType[12];
    float PosVector_x;
    float PosVector_y;
    char  PlainText[128];
    int   fontsize;
    uint32_t  Check;
};
static struct myosd_text myosd[3];
static int CHECK_ok(struct myosd_text *params)
{
    uint32_t i,len,chk=0;
    uint8_t *p;

    if(NULL == params)return 0;
    if(0x5a != params->Head && 0xa5 != params->Head)return 0;

    p = (uint8_t *)params;

    len = sizeof(struct myosd_text) - 4;
    for(i=0;i<len;i++)
    {
        chk += *p;
        p++;
    }
    if( chk == params->Check) return 1;

    return 0;
}

#if 0
static char *S_WEEK[7]   = {"������", "����һ", "���ڶ�", "������",
							"������", "������", "������"};
#else
static char *S_WEEK[7]   = {"SUN", "MON", "TUE", "WED",
							"THU", "FRI", "SAT"};
#endif
pthread_mutex_t gOsdMutex   = PTHREAD_MUTEX_INITIALIZER;
unsigned int           nOsdRebuild = 0;

HH_OSD_LOGO* HH_OSD_GetLogoHandle(int nCh, int bMin, int nOsdType)
{
	int nIndex      = 0;
	int nEncMaxType = 3;

	if (nCh < 0 ||  bMin < 0 || bMin >= nEncMaxType
		|| nOsdType < 0 || nOsdType >= HH_MAX_OSD_PER_GROUP)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_GetLogoHandle param err\n");
		return NULL;
	}

	nIndex = nCh * nEncMaxType *  HH_MAX_OSD_PER_GROUP +
			 bMin * HH_MAX_OSD_PER_GROUP + nOsdType;

	if (nIndex >= HH_MAX_OSD_NUM)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_GetLogoHandle index err %d\n", nIndex);
		return NULL;
	}
	//COMM_SYSLOG(LOG_ERR,"\nHH_OSD_GetLogoHandle nCh=%d,bMin=%d,nOsdType=%d,nIndex=%d,pGOsdLog[%d]=%d\n",nCh,bMin,nOsdType,nIndex,nIndex,pGOsdLog[nIndex]);
	return 	pGOsdLog[nIndex];
}

float HH_OSD_GetOsdRateX(int nCh, int nMinEnc)
{
	int	nEncWidth  = 0;
	int	nEncHeight = 0;
	int nEncType   = 0;

	//HH_VENC_GetVEncSize(nCh, nMinEnc, &nEncWidth, &nEncHeight, &nEncType);

	if(nMinEnc == 0)
	{
		nEncWidth = 1280 ;

	}
	else if(nMinEnc == 1)
	{
		//nEncWidth = 640 ;
		nEncWidth = 960 ;

	}
	//COMM_SYSLOG(LOG_ERR,"nEncWidth = %d" , nEncWidth);
    return (float)nEncWidth / 640;
}

float HH_OSD_GetOsdRateY(int nCh, int nMinEnc)
{
	int	nEncWidth  = 0;
	int	nEncHeight = 0;
	int nEncType   = 0;

	if(nMinEnc == 0)
	{
		 return (float)3;

	}
	else if(nMinEnc == 1)
	{
		return (float)1;

	}
}

int HH_OSD_GetOrg(int nCh, int nEnc, int osdType, HI_OSD_ORG  *pOsdOrg)
{
	int	nEncWidth  = 0;
	int	nEncHeight = 0;

	if (pOsdOrg == NULL || nCh < 0  || osdType < 0 ||
		osdType >= HH_MAX_OSD_PER_GROUP)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_GetOrg Param Err\n");
		return -1;
	}
	if(nEnc == 0)
	{
		switch(osdType)
		{
			case HH_OSD_TYPE_TIME:
				pOsdOrg->nOrgX = pvideo_osd_status->stDateOSD.X	;//0
				pOsdOrg->nOrgY = pvideo_osd_status->stDateOSD.Y	 ;//0					
				break;

			case HH_OSD_TYPE_TITLE:
				if(strcmp(myosd[0].PosType,"LowerLeft") == 0)
				{
					pOsdOrg->nOrgX = 0	;//0
					pOsdOrg->nOrgY = 1080 - 64	 ;//0					
				}
				else if(strcmp(myosd[0].PosType,"LowerRight") == 0)
				{
					pOsdOrg->nOrgX = 1920 -400	;//0
					pOsdOrg->nOrgY = 1080 - 64	 ;//0					

				}
				else if(strcmp(myosd[0].PosType,"UpperLeft") == 0)
				{
					pOsdOrg->nOrgX = 0	;//0
					pOsdOrg->nOrgY = 0	 ;//0					

				}
				else if(strcmp(myosd[0].PosType,"UpperRight") == 0)
				{
					pOsdOrg->nOrgX = 1920 -400	;//0
					pOsdOrg->nOrgY = 0	 ;//0					

				}
				else if(strcmp(myosd[0].PosType,"Custom") == 0)
				{
					pOsdOrg->nOrgX = myosd[0].PosVector_x*192/10;//0
					pOsdOrg->nOrgY = myosd[0].PosVector_y*108/10;//0					

				}
				else
				{
					pOsdOrg->nOrgX = pvideo_osd_status->stTitleOSD.X	;//0
					pOsdOrg->nOrgY	= pvideo_osd_status->stTitleOSD.Y;//pvideo_osd_status->stTitleOSD.Y + ipc_conf.height - 48					
				}
				break;

			case HH_OSD_TYPE_TITLE2:
				if(strcmp(myosd[1].PosType,"LowerLeft") == 0)
				{
					pOsdOrg->nOrgX = 0	;//0
					pOsdOrg->nOrgY = 1080 - 64	 ;//0					
				}
				else if(strcmp(myosd[1].PosType,"LowerRight") == 0)
				{
					pOsdOrg->nOrgX = 1920 -400	;//0
					pOsdOrg->nOrgY = 1080 - 64	 ;//0					

				}
				else if(strcmp(myosd[1].PosType,"UpperLeft") == 0)
				{
					pOsdOrg->nOrgX = 0	;//0
					pOsdOrg->nOrgY = 0	 ;//0					

				}
				else if(strcmp(myosd[1].PosType,"UpperRight") == 0)
				{
					pOsdOrg->nOrgX = 1920 -400	;//0
					pOsdOrg->nOrgY = 0	 ;//0					

				}
				else if(strcmp(myosd[1].PosType,"Custom") == 0)
				{
					pOsdOrg->nOrgX = myosd[1].PosVector_x*192/10;//0
					pOsdOrg->nOrgY = myosd[1].PosVector_y*108/10;//0					

				}
				else
				{
					pOsdOrg->nOrgX = pvideo_osd_status->stTitleOSD2.X	;//0
					pOsdOrg->nOrgY	= pvideo_osd_status->stTitleOSD2.Y;//pvideo_osd_status->stTitleOSD.Y + ipc_conf.height - 48
				}			
				break;

			case HH_OSD_TYPE_TITLE3:
				if(strcmp(myosd[2].PosType,"LowerLeft") == 0)
				{
					pOsdOrg->nOrgX = 0	;//0
					pOsdOrg->nOrgY = 1080 - 64	 ;//0					
				}
				else if(strcmp(myosd[2].PosType,"LowerRight") == 0)
				{
					pOsdOrg->nOrgX = 1920 -400	;//0
					pOsdOrg->nOrgY = 1080 - 64	 ;//0					

				}
				else if(strcmp(myosd[2].PosType,"UpperLeft") == 0)
				{
					pOsdOrg->nOrgX = 0	;//0
					pOsdOrg->nOrgY = 0	 ;//0					

				}
				else if(strcmp(myosd[2].PosType,"UpperRight") == 0)
				{
					pOsdOrg->nOrgX = 1920 -400	;//0
					pOsdOrg->nOrgY = 0	 ;//0					

				}
				else if(strcmp(myosd[2].PosType,"Custom") == 0)
				{
					pOsdOrg->nOrgX = myosd[2].PosVector_x*192/10;//0
					pOsdOrg->nOrgY = myosd[2].PosVector_y*108/10;//0					

				}
				else
				{
					pOsdOrg->nOrgX = pvideo_osd_status->stTitleOSD3.X	;//0
					pOsdOrg->nOrgY	= pvideo_osd_status->stTitleOSD3.Y;//pvideo_osd_status->stTitleOSD.Y + ipc_conf.height - 48
				}
				break;

		}

	}else if(nEnc == 1)
	{
		switch(osdType)
		{
			case HH_OSD_TYPE_TIME:
				pOsdOrg->nOrgX = pvideo_osd_status->stDateOSD.X	;
				pOsdOrg->nOrgY = pvideo_osd_status->stDateOSD.Y ;
				break;

			case HH_OSD_TYPE_TITLE:
				pOsdOrg->nOrgX = pvideo_osd_status->stTitleOSD.X;
				pOsdOrg->nOrgY	=pvideo_osd_status->stTitleOSD.Y + ipc_conf.height - 32;
				break;
		}
	}
	//COMM_SYSLOG(LOG_DEBUG,">>>>>pOsdOrg->nOrgX = %d pOsdOrg->nOrgY = %d \n ",pOsdOrg->nOrgX ,pOsdOrg->nOrgX);
	return 0;
}

int HH_OSD_GetColor(int nCh, HI_OSD_COLOR *pOsdColor)
{
	if (nCh < 0 || pOsdColor == NULL)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_GetColor Param Err\n");
		return -1;
	}

	pOsdColor->bgAlpha = 0;					//������Ϊ͸��
	pOsdColor->fgAlpha = 0x7F;               //ǰ����͸��
	 //��ɫ
	if (pvideo_osd_status->nOsdColor == 1)
	{
		pOsdColor->fgColor  = HH_OSD_COLOR_BLACK;
		pOsdColor->edgeColor= HH_OSD_COLOR_WHITE;
		pOsdColor->selColor = HH_OSD_COLOR_WHITE;
	}//��ɫ
	else if (pvideo_osd_status->nOsdColor == 2)
	{
		pOsdColor->fgColor  = HH_OSD_COLOR_YELLOW;
		pOsdColor->edgeColor= HH_OSD_COLOR_BLACK;
		pOsdColor->selColor = HH_OSD_COLOR_WHITE;
	}//��ɫ
	else if (pvideo_osd_status->nOsdColor == 3)
	{
		pOsdColor->fgColor  = HH_OSD_COLOR_RED;
		pOsdColor->edgeColor= HH_OSD_COLOR_BLACK;
		pOsdColor->selColor = HH_OSD_COLOR_WHITE;
	}//��ɫ
	else if (pvideo_osd_status->nOsdColor == 4)
	{
		pOsdColor->fgColor  = HH_OSD_COLOR_BLUE;
		pOsdColor->edgeColor= HH_OSD_COLOR_BLACK;
		pOsdColor->selColor = HH_OSD_COLOR_WHITE;
	}
	else
	{
		pOsdColor->fgColor  = HH_OSD_COLOR_WHITE;
		pOsdColor->edgeColor= HH_OSD_COLOR_BLACK;
		pOsdColor->selColor = HH_OSD_COLOR_BLACK;
		pOsdColor->bgColor = 64;
	}
	pOsdColor->bgColor = pOsdColor->fgColor; //������Ϊ͸�� ��ɫ���д
	return 0;
}

int HH_OSD_GetMenuColor(int nCh, HI_OSD_COLOR *pOsdColor)
{
	if (nCh != 0 || pOsdColor == NULL)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_GetMenuColor Param Err\n");
		return -1;
	}

	pOsdColor->bgAlpha = 0x7F;//������Ϊ͸��
	pOsdColor->fgAlpha = 0x7F;//ǰ����͸��

	pOsdColor->fgColor  = HH_OSD_COLOR_WHITE;
	pOsdColor->edgeColor= HH_OSD_COLOR_BLACK;
	pOsdColor->selColor = HH_OSD_COLOR_YELLOW;

	//pOsdColor->bgColor = pOsdColor->fgColor; //������Ϊ͸�� ��ɫ���д
	pOsdColor->bgColor = HH_OSD_COLOR_BLACK;
	return 0;
}


int HH_OSD_GetTimeTitle(int nCh, int bMin, char *pTitle)
{

	time_t timep;
	struct tm *pLocalTime;
	time(&timep);
	pLocalTime = localtime(&timep);

	if(pvideo_osd_status->stDateOSD.Show ||
	   pvideo_osd_status->stTimeOSD.Show ||
	   pvideo_osd_status->stWeekOSD.Show )
	{
		if (pvideo_osd_status->stDateOSD.Show )
		{
			if (pvideo_osd_status->nOsdPrefer == 0){
                		sprintf(pTitle,"%04d-%02d-%02d",1900+pLocalTime->tm_year,1+pLocalTime->tm_mon, pLocalTime->tm_mday) ;
			}else if (pvideo_osd_status->nOsdPrefer == 1){
				sprintf(pTitle,"%02d-%02d-%04d",1+pLocalTime->tm_mon,pLocalTime->tm_mday, 1900+pLocalTime->tm_year) ;
			}else{
				sprintf(pTitle,"%02d-%02d-%04d",pLocalTime->tm_mday,1+pLocalTime->tm_mon, 1900+pLocalTime->tm_year) ;
			}

		}

		if(pvideo_osd_status->stTimeOSD.Show )
		{
			sprintf(pTitle,"%s %02d:%02d:%02d %s",pTitle, pLocalTime->tm_hour, pLocalTime->tm_min, pLocalTime->tm_sec, S_WEEK[pLocalTime->tm_wday % 7] ) ;
		}

		//if(pvideo_osd_status->stWeekOSD.Show )
		//{
		//	sprintf(pTitle,"%s ",pTitle, S_WEEK[curTime.week % 7]);
		//}
	}
	return 0;
}

int HH_OSD_GetFrameTitle(int nCh, int bMin, char *pTitle)
{
    //extern VideoEnc_Info _VideoEncInfo[MAX_VIDEO_NUM];
	bMin = bMin % 2;

	if(pvideo_osd_status->stBitrateOSD.Show)
	{
	//
	}
	return 0;

}


int HH_OSD_GetPTZTitle(int nCh, int bMin, char *pTitle)
{

	return 0;
}
int HH_OSD_GetChNameTitle(int nCh, int bMin,char *pTitle)
{
    if(pvideo_osd_status->stTitleOSD.Show)
    {
		sprintf(pTitle,"%s",pvideo_osd_status->Title);
		//printf("$$$$$$$$$######pTitle[%s]\n",pTitle);

    }
	return 0;
}

int HH_OSD_GetTitle(int nCh, int bMin, int nOsdType, char *pTitle)
{
	if (nOsdType == HH_OSD_TYPE_TIME)
	{
		HH_OSD_GetTimeTitle(nCh, bMin, pTitle);// time
	}
	else if (nOsdType == HH_OSD_TYPE_TITLE)
	{
		//HH_OSD_GetChNameTitle(nCh, bMin, pTitle);//�ַ�
		if(CHECK_ok(&myosd[0]) != 0 && myosd[0].token[0] != 0)
		sprintf(pTitle,"%s",myosd[0].PlainText);
		else
		sprintf(pTitle," ");
		
	}
	else if (nOsdType == HH_OSD_TYPE_TITLE2)
	{
		//HH_OSD_GetChNameTitle(nCh, bMin, pTitle);//�ַ�
		if(CHECK_ok(&myosd[1]) != 0 && myosd[1].token[0] != 0)
		sprintf(pTitle,"%s",myosd[1].PlainText);
		else
		sprintf(pTitle," ");

	}
	else if (nOsdType == HH_OSD_TYPE_TITLE3)
	{
		//HH_OSD_GetChNameTitle(nCh, bMin, pTitle);//�ַ�
		if(CHECK_ok(&myosd[2]) != 0 && myosd[2].token[0] != 0)
		sprintf(pTitle,"%s",myosd[2].PlainText);
		else
		sprintf(pTitle," ");

	}

	return 0;
}

int HH_OSD_GetFontInt(int nCh, int bMin, int nOsdType)
{
	if (nOsdType == HH_OSD_TYPE_TITLE)
	{
		return 1;
	}
	else if (nOsdType == HH_OSD_TYPE_TITLE2)
	{
		return 1;
	}
	else if (nOsdType == HH_OSD_TYPE_TITLE3)
	{
		return 1;
	}
	
	return 0;
}

int HH_OSD_GetShow(int nCh, int bMin, int nOsdType)
{
	if (nOsdType == HH_OSD_TYPE_TIME)
	{
		return (pvideo_osd_status->stTimeOSD.Show ||
				pvideo_osd_status->stDateOSD.Show ||
				pvideo_osd_status->stWeekOSD.Show ||
				pvideo_osd_status->stBitrateOSD.Show);
	}
	else if (nOsdType == HH_OSD_TYPE_TITLE)
	{
		return pvideo_osd_status->stTitleOSD.Show;
	}
	else if (nOsdType == HH_OSD_TYPE_TITLE2)
	{
		return pvideo_osd_status->stTitleOSD.Show;
	}
	else if (nOsdType == HH_OSD_TYPE_TITLE3)
	{
		return pvideo_osd_status->stTitleOSD.Show;
	}

	return 0;
}

int HH_OSD_Show_Refresh(int nCh, int nMinEnc, int osdType)
{
	HH_OSD_LOGO *pOsdLogo    = NULL;
	int          showsStatus = 0;
	if (nCh < 0  ||  nMinEnc < 0 || nMinEnc > 2
		|| osdType < 0 || osdType >= HH_MAX_OSD_PER_GROUP)
	{
		COMM_SYSLOG(LOG_ERR,"HH_Check_Show_Refresh param err\n");
		return -1;
	}

	if ((pOsdLogo = HH_OSD_GetLogoHandle(nCh, nMinEnc, osdType)) == NULL)
	{
		COMM_SYSLOG(LOG_ERR,"HH_Check_Show_Refresh Get Handle Err\n");
		return -2;
	}

	showsStatus = HH_OSD_GetShow(nCh, nMinEnc, osdType);
	if (showsStatus != pOsdLogo->nStatus)
	{
		HI_OSD_Set_Show(pOsdLogo, showsStatus);
		pOsdLogo->nStatus = showsStatus; //2010-07-12 �޸Ĳ������ص�����
	}
	return 0;
}

int HH_OSD_ChName_Refresh(int nCh, int nMinEnc, int osdType)
{
	HH_OSD_LOGO *pOsdLogo                 = NULL;
	char   szTitle[HH_MAX_TITLE_NUM]= {0};
	int          nNeedReCreate            = 0;
	int          ret                      = 0;
	HI_OSD_ORG   OsdOrg;
	if (nCh < 0  ||  nMinEnc < 0 || nMinEnc > 2)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_ChName_Refresh param err\n");
		return -1;
	}

	if ((pOsdLogo = HH_OSD_GetLogoHandle(nCh, nMinEnc, osdType)) == NULL)
	{
		//COMM_SYSLOG(HH_LOG_ERR,"HH_OSD_ChName_Refresh Get Handle Err nCh=%d,osdType=%d\n", nCh, osdType);
		return -2;
	}

	memset(szTitle, 0, sizeof(szTitle));
	HH_OSD_GetTitle(nCh, nMinEnc, osdType, szTitle);
	HH_OSD_GetOrg(0,0, osdType, &OsdOrg);

	if (strncmp(szTitle, pOsdLogo->szOsdTitle, HH_MAX_TITLE_NUM) != 0 ||
		HI_OSD_Check_ImaSizeChange(pOsdLogo) || pOsdLogo->osdPos.nOrgX != OsdOrg.nOrgX || pOsdLogo->osdPos.nOrgY != OsdOrg.nOrgY )
	{
		memcpy(pOsdLogo->szOsdTitle, szTitle, HH_MAX_TITLE_NUM - 1);
		nNeedReCreate |= HI_OSD_Parse_OsdTitle(pOsdLogo);
		nNeedReCreate |= HI_OSD_Get_BmpSize(pOsdLogo);

		if (nNeedReCreate)
		{
			HI_OSD_Destroy(pOsdLogo);
			//COMM_SYSLOG(LOG_ERR,"111111111111111111111111111111111111111111111111\n");
			if ((ret = HI_Create_Osd_Reg(pOsdLogo)) != 0)
			{
				COMM_SYSLOG(LOG_ERR,"HH_ChName_Refresh Create Reg Err %x\n", ret);
				return ret;
			}
			HI_OSD_Refresh_Org(pOsdLogo);
			//HI_OSD_Refresh_Color(pOsdLogo);
		}

		if ((ret = HI_OSD_Create_Bitmap(pOsdLogo)) != 0)
		{
			COMM_SYSLOG(LOG_ERR,"HH_ChName_Refresh Create Bitmap Err %x\n", ret);
			return ret;
		}
		HH_OSD_Show_Refresh(nCh, nMinEnc, osdType);
	}


	return 0;
}

int HH_OSD_ChnAllName_Refresh(int nCh)
{
	int nEncType = 0;

	if (nCh < 0 )
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_ChnAllName_Refresh param err\n");
		return -1;
	}

	for (nEncType = 0; nEncType < VIDEO_NUM; ++nEncType)
	{
		HH_OSD_ChName_Refresh(nCh, nEncType, HH_OSD_TYPE_TITLE);
		HH_OSD_ChName_Refresh(nCh, nEncType, HH_OSD_TYPE_TITLE2);
		HH_OSD_ChName_Refresh(nCh, nEncType, HH_OSD_TYPE_TITLE3);
	}
	return 0;
}

int HH_OSD_AllName_Refresh()
{
	int nCh      = 0;

	for (nCh = 0; nCh < VIDEO_NUM; ++nCh)
	{
		HH_OSD_ChnAllName_Refresh(nCh);
		//COMM_SYSLOG(LOG_ERR,"111111\n");
	}
	return 0;
}

int HH_OSD_ChnTime_Refresh(int nCh)
{
	int nEncType = 0;

	for (nEncType = 0; nEncType < VIDEO_NUM; ++nEncType)
	{
		HH_OSD_ChName_Refresh(nCh, nEncType, HH_OSD_TYPE_TIME);
	}
	return 0;
}

int HH_OSD_AllTime_Refresh()
{
	int nCh      = 0;

	for (nCh = 0; nCh < VIDEO_NUM; ++nCh)
	{
		HH_OSD_ChnTime_Refresh(nCh);
	}
	return 0;
}

int HH_OSD_Org_Refresh(int nCh, int nEnc, int osdType)
{
	HH_OSD_LOGO *pOsdLogo = NULL;

	if (nCh < 0 ||  nEnc < 0 || nEnc > 2
		|| osdType < 0 || osdType >= HH_MAX_OSD_PER_GROUP)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_Org_Refresh param err\n");
		return -1;
	}

	if ((pOsdLogo = HH_OSD_GetLogoHandle(nCh, nEnc, osdType)) == NULL)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_Org_Refresh Get Handle Err\n");
		return -2;
	}

	HH_OSD_GetOrg(nCh, nEnc, osdType, &pOsdLogo->osdPos);
	return HI_OSD_Refresh_Org(pOsdLogo);
}

int HH_OSD_Color_Refresh(int nCh, int nEnc, int osdType)
{
	HH_OSD_LOGO *pOsdLogo                 = NULL;

	if (nCh < 0   ||  nEnc < 0 || nEnc > 2
		|| osdType < 0 || osdType >= HH_MAX_OSD_PER_GROUP)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_Color_Refresh param err\n");
		return -1;
	}

	if ((pOsdLogo = HH_OSD_GetLogoHandle(nCh, nEnc, osdType)) == NULL)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_Org_Refresh Get Handle Err\n");
		return -2;
	}

	HH_OSD_GetColor(nCh, &pOsdLogo->osdColor);

	return HI_OSD_Refresh_Color(pOsdLogo);
}

int HH_OSD_ChnColor_Refresh(int nCh)
{
	int nEncType = 0;

	if (nCh < 0 )
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_ChnColor_Refresh param err\n");
		return -1;
	}

	for (nEncType = 0; nEncType < 3; ++nEncType)
	{
		HH_OSD_Color_Refresh(nCh, nEncType, HH_OSD_TYPE_TIME);
		HH_OSD_Color_Refresh(nCh, nEncType, HH_OSD_TYPE_TITLE);
	}
	return 0;
}

int HH_OSD_ChnOrg_Refresh(int nCh)
{
	int nEncType = 0;

	if (nCh < 0)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_ChnAllName_Refresh param err\n");
		return -1;
	}

	for (nEncType = 0; nEncType < 3; ++nEncType)
	{
		HH_OSD_Org_Refresh(nCh, nEncType, HH_OSD_TYPE_TIME);
		HH_OSD_Org_Refresh(nCh, nEncType, HH_OSD_TYPE_TITLE);
	}
	return 0;
}

int HH_OSD_ChnShow_Refresh(int nCh)
{
	int nEncType = 0;

	if (nCh < 0 )
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_ChnAllName_Refresh param err\n");
		return -1;
	}

	for (nEncType = 0; nEncType < 2; ++nEncType)
	{
		HH_OSD_Show_Refresh(nCh, nEncType, HH_OSD_TYPE_TIME);
		HH_OSD_Show_Refresh(nCh, nEncType, HH_OSD_TYPE_TITLE);
	}
	return 0;
}
int HH_OSD_Destory(int nCh, int nEnc, int osdType)
{
	HH_OSD_LOGO *pOsdLogo = NULL;

	if (nCh < 0  ||  nEnc < 0 || nEnc > 2
		|| osdType < 0 || osdType >= HH_MAX_OSD_PER_GROUP)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_Org_Refresh param err\n");
		return -1;
	}

	if ((pOsdLogo = HH_OSD_GetLogoHandle(nCh, nEnc, osdType)) == NULL)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_Org_Refresh Get Handle Err\n");
		return -2;
	}

	memset(pOsdLogo->szOsdTitle, 0, HH_MAX_TITLE_NUM);
	return HI_OSD_Destroy(pOsdLogo);
}

int HH_OSD_Rebuild(int nCh, int nEnc, int osdType)
{
	HH_OSD_LOGO *pOsdLogo = NULL;

	if (nCh < 0  ||  nEnc < 0 || nEnc > 2
		|| osdType < 0 || osdType >= HH_MAX_OSD_PER_GROUP)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_Org_Refresh param err\n");
		return -1;
	}

	if ((pOsdLogo = HH_OSD_GetLogoHandle(nCh, nEnc, osdType)) == NULL)
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_Org_Refresh Get Handle Err\n");
		return -2;
	}

	HH_OSD_GetOrg(nCh, nEnc, osdType, &pOsdLogo->osdPos);
	HH_OSD_GetColor(nCh, &pOsdLogo->osdColor);
	HH_OSD_GetTitle(nCh, nEnc, osdType, pOsdLogo->szOsdTitle);
	pOsdLogo->nStatus = HH_OSD_GetShow(nCh, nEnc, osdType);

	return HI_OSD_Build(pOsdLogo);
}

int HH_OSD_ChnReBuild(int nCh)
{
	int nEncType = 0;

	if (nCh < 0 )
	{
		COMM_SYSLOG(LOG_ERR,"HH_OSD_ChnAllName_Refresh param err\n");
		return -1;
	}
	nOsdRebuild = 1;
	for (nEncType = 0; nEncType < 3; ++nEncType)
	{
		HH_OSD_Destory(nCh, nEncType, HH_OSD_TYPE_TIME);
		HH_OSD_Destory(nCh, nEncType, HH_OSD_TYPE_TITLE);
	}
	for (nEncType = 0; nEncType < 3; ++nEncType)
	{
		HH_OSD_Rebuild(nCh, nEncType, HH_OSD_TYPE_TIME);
		HH_OSD_Rebuild(nCh, nEncType, HH_OSD_TYPE_TITLE);
	}
	nOsdRebuild = 0;

	return 0;
}



static int InitOsdProSync = 0;
static int listen_fd;
static int com_fd;
static int ProSyncState;
static pthread_t OSDProSyncPid;
//����ͨ��ͬ������osd

/**************************************************************
 * �ж��Ƿ��Ѿ����ӣ�����������ȴ�������Ϣ��
 * �������ӻ����ӶϿ���ȴ�����
 **************************************************************/
HI_VOID* OSDProSync_Process(HI_VOID *p)
{
	int len;		
	int ret;
	struct sockaddr_un clt_addr;
	struct myosd_text rmyosd[3];		
	do
	{
		ret=sizeof(clt_addr);
		printf("RTSP��APP�ȴ�����...\n");
		com_fd=accept(listen_fd,(struct sockaddr*)&clt_addr,&ret);
		printf("com_fd - %d\n",com_fd);
	} while (com_fd<0);
	printf("RTSP��APP���ӵ�ONVIF\n");
	while (1)
	{
		len=read(com_fd,(char *)rmyosd,sizeof(rmyosd));		
		if(len != 0) 
		{
			if(CHECK_ok(&rmyosd[0]) != 0 && CHECK_ok(&rmyosd[1]) != 0 && CHECK_ok(&rmyosd[2]) != 0)
			{
				memcpy(myosd,rmyosd,sizeof(myosd));
				printf("GET NEW OSD\n");
					
			}
			else
			{
				printf("Message from client %d\n",len);
				printf("OSD CHECK ERROR!\n");
			}						
		}
		else
		{
			printf("\r\nRTSP��APP��ONVIF���ӶϿ�\r\n");
			close(com_fd);
			close(listen_fd);
			while(1)
			{
				sleep(3600);
			}		
			/* code */
		}	
	}
	
}

static int Init_osd_prosync(void)
{
	if(InitOsdProSync == 0)
	{
		struct sockaddr_un srv_addr;
		int ret;
		
		listen_fd=socket(PF_UNIX,SOCK_STREAM,0);
		printf("steup 1\n");
		if(listen_fd<0) 
		{ 		
			return 0; 
		}
		srv_addr.sun_family=AF_UNIX;
		strncpy(srv_addr.sun_path,UNIX_DOMAIN,sizeof(srv_addr.sun_path)-1);
		unlink(UNIX_DOMAIN);
		ret=bind(listen_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
		printf("steup 2\n");
		if(ret==-1) 
		{ 
			close(listen_fd); 
			unlink(UNIX_DOMAIN); 
			return 0; 
		} 
		ret=listen(listen_fd,1); 
		printf("steup 3\n");
		if(ret==-1) 
		{ 			
			close(listen_fd); 
			unlink(UNIX_DOMAIN); 
			return 0; 
		} 
		printf("steup 4\n");

		pthread_create(&OSDProSyncPid, 0, OSDProSync_Process, NULL);		
		InitOsdProSync = 1; 
	}
	return InitOsdProSync;
}

int HH_OSD_All_Refresh()
{
	if (nOsdRebuild == 0)
	{
		HH_OSD_AllTime_Refresh();
		HH_OSD_AllName_Refresh();
		Init_osd_prosync();
	}
	return 0;
}


/**************************************************************
* ��������	 HH_OSD_SetOsdPosDefault
* ��������:   ����OSDĬ�����꣬��OSD�ص�ʱ����
* �������:
* �������:
* ����ֵ:
* ����:	 by yj 2012-09-17
**************************************************************/

int HH_OSD_SetOsdPosDefault()
{
	int nCh      = 0;
	pvideo_osd_status->stDateOSD.X			= 1920-400;//0
	pvideo_osd_status->stDateOSD.Y			= 0;//32
	pvideo_osd_status->stTitleOSD.X			= 0;//0
	pvideo_osd_status->stTitleOSD.Y			= 0;//32

	pvideo_osd_status->stTitleOSD2.X			= 0;//0
	pvideo_osd_status->stTitleOSD2.Y			= 1080-128;//32
	pvideo_osd_status->stTitleOSD3.X			= 1920-400;//0
	pvideo_osd_status->stTitleOSD3.Y			= 1080-128;//32

	pvideo_osd_status->nOsdColor = 5;
	pvideo_osd_status->stDateOSD.Show = 1;
	pvideo_osd_status->stTimeOSD.Show = 1;
	pvideo_osd_status->stWeekOSD.Show = 1 ;
	pvideo_osd_status->nOsdPrefer = 0 ;
	pvideo_osd_status->stBitrateOSD.Show = 0;
	pvideo_osd_status->stTitleOSD.Show  = 1 ;

	pvideo_osd_status->stTitleOSD2.Show  = 1 ;
	pvideo_osd_status->stTitleOSD3.Show  = 1 ;

	if(strlen(pvideo_osd_status->Title) == 0)
	{	//�����޸����Ͻǵ����֣�������ʾ��Ӣ�ģ�
		sprintf(pvideo_osd_status->Title,"%s",	 "  ��Ϊ��˼HI35xx���������IPCʵ��ƽ̨\r\n  www.ifengzai.cn");
	}

	ipc_conf.width = 1920;
	ipc_conf.height = 1080;

	return 0;
}


int HH_OSD_SetOsdParams(int Osd_type,int Osd_Show)
{
	pvideo_osd_status->stDateOSD.Show = Osd_Show;
	pvideo_osd_status->stTimeOSD.Show = Osd_Show;
	pvideo_osd_status->stWeekOSD.Show = Osd_Show;
	pvideo_osd_status->nOsdPrefer = Osd_type;
	return 0 ;

}

int HH_OSD_unInit()
{
	free(pvideo_osd_status);
	return 0 ;
}

int HH_OSD_Init()
{
	int 		 nCh      = 0;
	int 		 nEncType = 0;//  stream_chn0
	int 		 osdType  = 0;
	int 		 index    = 0;
	HI_OSD_ORG	 osdOrg;
	HI_OSD_COLOR osdColor;
	char         szTitle[HH_MAX_TITLE_NUM] = {0};
	COMM_SYSLOG(LOG_DEBUG,"HH_OSD_Init Start...\n");
	//ʵ����Ŀ��ʹ�õĽṹ�壬��������OSD�Ŀ���
	pvideo_osd_status = (VIDEO_OSD_STATUS *)malloc(sizeof(VIDEO_OSD_STATUS));
	//VIDEO_NUM  venc��chn��Ŀǰֻ��1·��Ƶ������ֱ��д1.
	//HH_MAX_OSD_PER_GROUP  OSD �������
//	for (nEncType = 0; nEncType < VIDEO_NUM; nEncType++) //  chanel 0
//	{
		for (osdType = 0; osdType < HH_MAX_OSD_PER_GROUP; osdType++)  // 2 ����
		{

			memset(&osdOrg,   0, sizeof(HI_OSD_ORG));
			memset(&osdColor, 0, sizeof(HI_OSD_COLOR));
			memset(szTitle,   0, sizeof(szTitle));

			HH_OSD_SetOsdPosDefault();

			HH_OSD_GetOrg(nCh, nEncType, osdType, &osdOrg);//��ȡ��ʾ������

			HH_OSD_GetColor(nCh, &osdColor);
			HH_OSD_GetTitle(nCh, nEncType, osdType, szTitle);
			
			//index  OSDת���ɾ����תΪΪ 0 �� 1��ʵ��ʹ��ֱ�� ���� osdType����
			index = nEncType * HH_MAX_OSD_PER_GROUP + osdType;//osdmenu
			//printf("index=========%d",index);
			//printf("szTitle=========%s",szTitle);
			if (NULL == HI_Create_Osd(index,&pGOsdLog[index], (VENC_GRP)nEncType, osdOrg, osdColor, szTitle, HH_OSD_GetFontInt(nCh, nEncType, osdType), HH_OSD_GetShow(nCh, nEncType, osdType)))
				COMM_SYSLOG(LOG_ERR,"HI_Create_Osd %d %d %d failed !\n", nCh,nEncType, osdType);
		}
//	}

	COMM_SYSLOG(LOG_DEBUG,"HH_OSD_Init End...\n");
	return 0;
}


