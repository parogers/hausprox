#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int parse_date_time(
	char *buf,
	int &year, int &month, int &day,
	int &hour, int &min, int &sec)
{
	const char *delims = ":-/ ";
	char *str;

	str = strtok(buf, delims);
	year = atoi(str);

	str = strtok(NULL, delims);
	month = atoi(str);

	str = strtok(NULL, delims);
	day = atoi(str);

	str = strtok(NULL, delims);
	hour = atoi(str);

	str = strtok(NULL, delims);
	min = atoi(str);

	str = strtok(NULL, delims);
	sec = atoi(str);
}

int main(void)
{
	char buf[128];
	memset(buf, 0, sizeof(buf));

	fgets(buf, sizeof(buf)-1, stdin);

	int year, month, day, hour, min, sec;
	parse_date_time(buf, year, month, day, hour, min, sec);

	printf("%d-%d-%d %d:%d:%d\n", year, month, day, hour, min, sec);
	return 0;
}
