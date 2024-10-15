// Copyright (c) 2024 The idump project authors. SPDX-License-Identifier: MIT.
// This file is part of idump(https://github.com/wichue/idump).

#include "CmdLineParse.h"
#ifdef WIN32
#include "getopt.h"
#else
#include <getopt.h>// for getopt_long
#endif

#include <stdlib.h>// for exit
#include <stdio.h>// for printf

#include "GlobalValue.h"
#include "Logger.h"

namespace chw {

#define MAX_TIME 86400
#define MIN_INTERVAL 0.1
#define MAX_INTERVAL 60.0

const double KILO_UNIT = 1024.0;
const double MEGA_UNIT = 1024.0 * 1024.0;
const double GIGA_UNIT = 1024.0 * 1024.0 * 1024.0;
const double TERA_UNIT = 1024.0 * 1024.0 * 1024.0 * 1024.0;

INSTANCE_IMP(CmdLineParse)

/* -------------------------------------------------------------------
 * unit_atoi
 *
 * Given a string of form #x where # is a number and x is a format
 * character listed below, this returns the interpreted integer.
 * Tt, Gg, Mm, Kk are tera, giga, mega, kilo respectively
 * ------------------------------------------------------------------- */
// 返回单位MB的整数
uint32_t unit_atoi(const char *s)
    {
	double    n;
	char      suffix = '\0';

	          assert(s != NULL);

	/* scan the number and any suffices */
	          sscanf(s, "%lf%c", &n, &suffix);

	/* convert according to [Tt Gg Mm Kk] */
	switch    (suffix)
	{
	case 't': case 'T':
	    n *= TERA_UNIT;
	    break;
	case 'g': case 'G':
	    n *= GIGA_UNIT;
	    break;
	case 'm': case 'M':
	    n *= MEGA_UNIT;
	    break;
	case 'k': case 'K':
	    n *= KILO_UNIT;
	    break;
	default:
	    break;
	}
	return (uint32_t) n;
}				/* end unit_atof */

/**
 * @brief 解析命令行参数
 * 
 * @param argc		[in]参数数量
 * @param argv		[in]参数指针
 * @return uint32_t 成功返回chw::success,失败返回chw::fail
 */
uint32_t CmdLineParse::parse_arguments(int argc, char **argv)
{
    static struct option longopts[] = 
    {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},

        {"server", no_argument, NULL, 's'},
        {"udp", no_argument, NULL, 'u'},
        {"version4", no_argument, NULL, '4'},
        {"version6", no_argument, NULL, '6'},

        {"Text", no_argument, NULL, 'T'},
        {"Press", no_argument, NULL, 'P'},
        {"File", no_argument, NULL, 'F'},

        {"port", required_argument, NULL, 'p'},
        {"client", required_argument, NULL, 'c'},
        {"time", required_argument, NULL, 't'},
        {"interval", required_argument, NULL, 'i'},
        {"bind", required_argument, NULL, 'B'},
        {"length", required_argument, NULL, 'l'},
        {"bandwidth", required_argument, NULL, 'b'},


        {NULL, 0, NULL, 0}
    };
    int flag;
    int portno;
   
    while ((flag = getopt_long(argc, argv, "hvsu46p:c:t:i:B:l:b:", longopts, NULL)) != -1) {
        switch (flag) {
            case 'h':
				help();
                exit(0);
                break;
            case 'v':
				version();
				exit(0);
                break;

            case 's':
                if (gConfigCmd.role == 'c') {
                    printf("cannot be both server and client\n");
                    return chw::fail;
                }
                gConfigCmd.role = 's';
                break;
            case 'u':
                gConfigCmd.protol = SockNum::Sock_UDP;
                break;
            case '4':
                gConfigCmd.domain = AF_INET;
                break;
            case '6':
                gConfigCmd.domain = AF_INET6;
                break;
            case 'p':
		        portno = atoi(optarg);
		        if (portno < 1 || portno > 65535) {
		            printf("Bad port number:%d\n",portno);
		            return chw::fail;
		        }
		        gConfigCmd.server_port = portno;
                break;
            case 'c':
                if (gConfigCmd.role == 's') {
                    printf("cannot be both server and client\n");
                    return chw::fail;
                }
                gConfigCmd.role = 'c';
                gConfigCmd.server_hostname = optarg;
                break;
            case 't':
                gConfigCmd.duration = atoi(optarg);
                if (gConfigCmd.duration > MAX_TIME || gConfigCmd.duration < 0) {
                    printf("invalid duration time:%d\n",gConfigCmd.duration);
                    return chw::fail;
                }
                break;
            case 'i':
                gConfigCmd.reporter_interval = atof(optarg);
                if ((gConfigCmd.reporter_interval < MIN_INTERVAL || gConfigCmd.reporter_interval > MAX_INTERVAL) && gConfigCmd.reporter_interval != 0) {
                    printf("Invalid report interval:%.2f (min = %.2f, max = %.2f seconds)\n",gConfigCmd.reporter_interval,MIN_INTERVAL,MAX_INTERVAL);
                    return -1;
                }
                break;
            case 'B':
                gConfigCmd.bind_address = optarg;
                if(isIP(gConfigCmd.bind_address) == false) {
                    printf("bind addr not a ip:%s\n",gConfigCmd.bind_address);
                    return chw::fail;
                }
                break;
            case 'l':
                gConfigCmd.blksize = atoi(optarg);
                break;
            case 'b':
                gConfigCmd.bandwidth = unit_atoi(optarg);
                break;

                
            default:
				printf("Incorrect parameter option, --help for help.\n");
                exit(1);
        }
    }

    if ((gConfigCmd.role != 'c') && (gConfigCmd.role != 's')) {
        printf("must either be a client (-c) or server (-s)\n");
        return chw::fail;
    }

    if(gConfigCmd.server_port == 0) {
        printf("server port is empty, use -p option, --help for help.\n");
        return chw::fail;
    }

    return chw::success;
}

/**
 * @brief 打印帮助
 */
void CmdLineParse::help()
{
	version();

	printf(
			"	--help(-h) for help.\n"
			"	--version(-v) for version info.\n\n"

			"	--file(-f), pcap file to parse,default model,without -c.\n"
			"	--json(-j), json match condition,from file.\n"
			"	--filter(-f), cmd line filter condition,like wireshark.\n"
			"	--save(-s), log output to file,without this option,log output to console.\n"
			"	--max(-m), print msg by bytes up to max,without this option or 0,do not print msg details.\n\n"

			"	--compare(-c),compare by byte,must option:file1,file2,can use --json and --filter conditions.\n"
			"	--file1(-a),one of pcap file for compare.\n"
			"	--file2(-b),one of pcap file for compare.\n"
			"	--start(-k), compare model,the begin offset of msg,without this option begin offset is 0.\n"
			"	--end(-l), compare model,the end offset of msg,without this option end offset is 0.\n"
			);

	exit(0);
}

/**
 * @brief 打印版本
 */
void CmdLineParse::version()
{
#ifdef WIN32
	printf("idump version 1.0.0 for windows.  Welcome to visit url: https://github.com/wichue/idump\n");
#else
	printf("idump version 1.0.0 for linux.  Welcome to visit url: https://github.com/wichue/idump\n");
#endif
}

}//namespace chw
