/*******************************************************************************
 * Copyright (c) 2012, 2016 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *    Ian Craggs - change delimiter option from char to string
 *    Al Stockdill-Mander - Version using the embedded C client
 *    Ian Craggs - update MQTTClient function names
 *******************************************************************************/

/*
 
 stdout subscriber
 
 compulsory parameters:
 
  topic to subscribe to
 
 defaulted parameters:
 
	--host localhost
	--port 1883
	--qos 2
	--delimiter \n
	--clientid stdout_subscriber
	
	--userid none
	--password none

 for example:

    stdoutsub topic/of/interest --host iot.eclipse.org

*/
#include <stdio.h>
#include <memory.h>
#include "MQTTClient.h"

#include <stdio.h>
#include <signal.h>

#include <sys/time.h>

// 当 toStop == 1 时结束当前程序
volatile int toStop = 0;

/**
 * 打印此例程的命令行启动参数
 */
void usage()
{
	printf("MQTT stdout subscriber\n");
	printf("Usage: stdoutsub topicname <options>, where options are:\n");
	printf("  --host <hostname> (default is localhost)\n");
	printf("  --port <port> (default is 1883)\n");
	printf("  --qos <qos> (default is 2)\n");
	printf("  --delimiter <delim> (default is \\n)\n");
	printf("  --clientid <clientid> (default is hostname+timestamp)\n");
	printf("  --username none\n");
	printf("  --password none\n");
	printf("  --showtopics <on or off> (default is on if the topic has a wildcard, else off)\n");
	exit(-1);
}

/**
 * 在Linux环境下收到kill -9信号或者Ctrl-C的处理函数
 * @param sig 注册的信号
 */
void cfinish(int sig)
{
	signal(SIGINT, NULL);
	toStop = 1;
}

/**
 * 声明一个结构体类型，并且定义一个全局变量 opts ，包含此客户端相关的信息：clientid, username, password, host, port
 */
struct opts_struct
{
	char* clientid;		// 客户端唯一标志
	int nodelimiter;	// 分隔符1
	char* delimiter;	// 分隔符2
	enum QoS qos;		// QoS 参数
	char* username;		// 用户名字
	char* password;		// 用户密码
	char* host;			// 服务器IP地址
	int port;			// 服务器端口号
	int showtopics;		// 如果为真则打印参数信息
} opts =
{
	(char*)"stdout-subscriber", 0, (char*)"\n", QOS2, NULL, NULL, (char*)"localhost", 1883, 0
};


/**
 * 分析命令行传递进来的参数，并且填充到全局变量 opts 中
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组指针
 */
void getopts(int argc, char** argv)
{
	int count = 2;
	
	while (count < argc)
	{
		if (strcmp(argv[count], "--qos") == 0) // 设定QoS级别
		{
			if (++count < argc)
			{
				if (strcmp(argv[count], "0") == 0)
					opts.qos = QOS0;
				else if (strcmp(argv[count], "1") == 0)
					opts.qos = QOS1;
				else if (strcmp(argv[count], "2") == 0)
					opts.qos = QOS2;
				else
					usage();
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "--host") == 0) 	// 设定服务器IP地址
		{
			if (++count < argc)
				opts.host = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--port") == 0)	// 设定服务器端口号
		{
			if (++count < argc)
				opts.port = atoi(argv[count]);
			else
				usage();
		}
		else if (strcmp(argv[count], "--clientid") == 0)	// 设定clientid
		{
			if (++count < argc)
				opts.clientid = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--username") == 0)	// 设定 username
		{
			if (++count < argc)
				opts.username = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--password") == 0)	// 设定 password
		{
			if (++count < argc)
				opts.password = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--delimiter") == 0)	// 设定分隔符
		{
			if (++count < argc)
				opts.delimiter = argv[count];
			else
				opts.nodelimiter = 1;
		}
		else if (strcmp(argv[count], "--showtopics") == 0)	// 设定是否显示参数
		{
			if (++count < argc)
			{
				if (strcmp(argv[count], "on") == 0)
					opts.showtopics = 1;
				else if (strcmp(argv[count], "off") == 0)
					opts.showtopics = 0;
				else
					usage();
			}
			else
				usage();
		}
		count++;
	}
	
}


void messageArrived(MessageData* md)
{
	MQTTMessage* message = md->message;

	if (opts.showtopics)
		printf("%.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data);
	if (opts.nodelimiter)
		printf("%.*s", (int)message->payloadlen, (char*)message->payload);
	else
		printf("%.*s%s", (int)message->payloadlen, (char*)message->payload, opts.delimiter);
	//fflush(stdout);
}


int main(int argc, char** argv)
{
	int rc = 0;
	// 需要用户先分配缓冲区，然后挂载到MQTTClient对象上
	unsigned char buf[100];
	unsigned char readbuf[100];
	
	if (argc < 2)
		usage();
	
	char* topic = argv[1];

	// 如果 topic 中包含 '#' 或者 '+' 才设置 showtopics = 1
	if (strchr(topic, '#') || strchr(topic, '+'))
		opts.showtopics = 1;
	if (opts.showtopics)
		printf("topic is %s\n", topic);

	getopts(argc, argv);

	// 代表网络连接的实例
	// 此实例包含1）网络描述符my_socket；2）发送网络数据报的方法XXXread()；3）读取网络数据报的方法XXXwrite()
	Network n;
	// 代表客户端的实例
	MQTTClient c;

	signal(SIGINT, cfinish);
	signal(SIGTERM, cfinish);

	// 注册mqttread(), mqttwrite() 函数
	NetworkInit(&n);
	// 套接字连接上远程服务器的指定端口号
	NetworkConnect(&n, opts.host, opts.port);
	// 挂载网络实例到 MQTTClient 上，并且初始化缓冲区资源，客户端参数等信息
	MQTTClientInit(&c, &n, 1000, buf, 100, readbuf, 100); // 命令间隔时间是1000ms
 
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	data.willFlag = 0;
	data.MQTTVersion = 3;
	data.clientID.cstring = opts.clientid;
	data.username.cstring = opts.username;
	data.password.cstring = opts.password;

	data.keepAliveInterval = 10;
	data.cleansession = 1;
	printf("Connecting to %s %d\n", opts.host, opts.port);

	// 上面设置了 MQTTClient的各项参数，连接到服务器
	rc = MQTTConnect(&c, &data);
	printf("Connected %d\n", rc);

    printf("Subscribing to %s\n", topic);
	rc = MQTTSubscribe(&c, topic, opts.qos, messageArrived);
	printf("Subscribed %d\n", rc);

	// 当 toStop == 1时，结束当前程序
	while (!toStop)
	{
		// 在调用MQTTYield() 函数之前，应该做好订阅、发布的工作，然后开始进入周期性的读取服务器的消息过程中
		// 多进程的操作不用那么复杂，只需要在这个 while 之前完成连接服务器、订阅主体、发布消息的动作，
		// 就可以用MQTTYield() 定期地去处理服务器消息
		// 周期性的读取一帧消息并做处理
		MQTTYield(&c, 1000);
	}

	printf("Stopping\n");

	MQTTDisconnect(&c);
	NetworkDisconnect(&n);

	return 0;
}


