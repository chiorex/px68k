// ---------------------------------------------------------------------------------------
//  COMMON - 標準ヘッダ群（COMMON.H）とエラーダイアログ表示とか
// ---------------------------------------------------------------------------------------
//#include	<windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef ANDROID
#include <android/log.h>
#endif
#include <sys/time.h>
#include <pthread.h>

//#include	"sstp.h"

//extern HWND hWndMain;
extern const char PrgTitle[];

// P6L: PX68K_LOG
//      ~ ~   ~
#define P6L_LEN 256
char p6l_buf[P6L_LEN];

static pthread_t main_thread;

void init_quit_if_main_thread()
{
	main_thread = pthread_self();
}

void quit_if_main_thread()
{

	if (pthread_self() == main_thread) {

		p6logd("Called from main thread!\n");
		exit(-1);

	}
}

void Error(const char* s)
{
	printf("%s Error: %s\n", PrgTitle, s);

//	SSTP_SendMes(SSTPMES_ERROR);

//	MessageBox(hWndMain, s, title, MB_ICONERROR | MB_OK);
}

// log for debug


void p6logd(const char *fmt, ...)
{
	va_list args;
	struct timeval when;
	static struct timeval start = { 0, 0};

	if ((start.tv_usec == 0) && (start.tv_sec == 0))
		gettimeofday(&start, NULL);

	gettimeofday(&when, NULL);
	timersub(&when, &start, &when);

	va_start(args, fmt);
	vsnprintf(p6l_buf, P6L_LEN, fmt, args);
	va_end(args);


#if defined(ANDROID)
	__android_log_write(ANDROID_LOG_DEBUG, "Tag", p6l_buf);
#elif defined(PSP)
	printf("%d s %ld: %s", when.tv_sec, when.tv_usec, p6l_buf);
#else
	printf("%d s %ld: %s", when.tv_sec, when.tv_usec, p6l_buf);
#endif
	fflush(stdout);
}
