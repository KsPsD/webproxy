/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */

/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void)
{
  char *buf, *p, *pp;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;
  //연습문제를 위해서 파싱 부분을 바꿈
  //
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    if ((p = strchr(buf, 'A')) == NULL) //여기서 A ,B 값이 있는지 확인하고 있다면 adder.html에서 form으로 받은 값들임 따라서
    {                                   //거기에 맞게 파싱을 해줬습니다.
      p = strchr(buf, '&');
      *p = '\0';
      strcpy(arg1, buf);
      strcpy(arg2, p + 1);
      n1 = atoi(arg1);
      n2 = atoi(arg2);
    }
    else //여긴 원래 주소창에서 GET으로 넘겨준 인자들
    {
      pp = strchr(buf, '&');
      *pp = '\0';
      strcpy(arg1, p + 2);
      // char *ppp = strchr(buf, 'B');
      strcpy(arg2, pp + 3);
      n1 = atoi(arg1);
      n2 = atoi(arg2);
    }
  }

  sprintf(content, "QUERY_STRI =%s", buf);
  sprintf(content, "Welcome to tiny: ");
  sprintf(content, "%sTHE Internet addeitioPortal .\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d =%d \r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting! \r\n", content);

  printf("Connection: close \r\n");
  printf("Conetnet-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);
}
/* $end adder */