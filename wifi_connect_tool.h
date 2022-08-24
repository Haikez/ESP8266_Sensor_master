#ifndef _WIFI_CONNECT_TOOL_H_
#define _WIFI_CONNECT_TOOL_H_
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <FS.h>

#define LED 2

int Signal_filtering = -200;
String Hostname = "ESP8266";

String smart_data = "";
int size_data = 0; //WIFI数据长度
String wifiname="";
String wifipsw="";
int wifiname_len; // wifi名的长度

void write_eeprom() //写入函数
{
  while (1)
  {
      wifiname_len = wifiname.length(); //获取wifi名的长度
      if (wifiname_len < 9) {//判断wifi名是否为个位数，是则在前面补零
        smart_data = String("0") + String(wifiname_len) + String(size_data) + String (wifiname.c_str()) + String(wifipsw.c_str());
        size_data = String(smart_data).length();
        smart_data = String("0") + String(wifiname_len) + String(size_data) + String (wifiname.c_str()) + String(wifipsw.c_str());
      } else {
        smart_data = String(wifiname_len) + String(size_data) + String (wifiname.c_str()) + String(wifipsw.c_str());
        size_data = String(smart_data).length();
        smart_data = String(wifiname_len) + String(size_data) + String (wifiname.c_str()) + String(wifipsw.c_str());
      }
      Serial.println(size_data);
      Serial.println(smart_data); //打印出保存在eeprom数据
      Serial.println(String(smart_data).length());//打印出保存在eeprom中的数据长度
      for (int addr = 300; addr < size_data + 301; addr++)
      {
        //int data = addr%256; //在该代码中等同于int data = addr;因为下面write方法是以字节为存储单位的.
        //String(smart_data).charAt(addr);
        EEPROM.write(addr, toascii(String(smart_data).charAt(addr-300))); //写数据
		delay(50);
	  }
      EEPROM.commit(); //保存更改的数据

      Serial.println("End write");
      break;
    }
	EEPROM.end();
}

void read_eeprom() //读取函数
{
  wifiname = "";
  wifipsw = "";
  int a = (int(EEPROM.read(300)) - int('0')) * 10 + (int(EEPROM.read(301)) - int('0')); //读数据，地址0和地址1分别保存wifi名称的长度的十位和个位
  int b = (int(EEPROM.read(302)) - int('0')) * 10 + (int(EEPROM.read(303)) - int('0')) ; //读数据，地址2和地址3分别保存wifi密码的长度的十位和个位
  Serial.println();
  Serial.println(a);//打印出wifi名的长度，看是否正确
  Serial.println(b);//打印出整个保存在eeprom中字符串的长度
  for (int addr = 304; addr < b + 301; addr++)//地址304开始则是wifi的名称，然后依次是wifi密码
  {
    int data = EEPROM.read(addr); //读数据
    if (addr < a + 304)
    {
      wifiname = wifiname + char(data);
    }
    else
    {
      wifipsw = wifipsw + char(data);
    }
    delay(2);
  }
  Serial.print("SSID:");
  Serial.println(wifiname);
  Serial.print("PASSWORD:");
  Serial.println(wifipsw);
}

String wifi_flag = "0";				  //配网标志
const char *AP_NAME = "智能灯控配网"; //wifi名字
String responseHTML = " <html><head><title>正在跳转</title></head><body><script language='javascript'>document.location = '/'</script></body></html>";
const byte DNS_PORT = 53;		//DNS端口号
IPAddress apIP(6, 6, 6, 6);		//esp8266-AP-IP地址
DNSServer dnsServer;			//创建dnsServer实例
ESP8266WebServer webServer(80); //创建WebServer

void wwwroot()
{

	if (wifi_flag == "1")
	{
		File file = SPIFFS.open("/index.html", "r");
		webServer.streamFile(file, "text/html");
		file.close();
	}
	else if (wifi_flag == "0")
	{
		Serial.println("网页加载完成");
		File file = SPIFFS.open("/config.html", "r");
		webServer.streamFile(file, "text/html");
		file.close();
	}
}

String wifi_type(int typecode)
{
	if (typecode == ENC_TYPE_NONE)
		return "Open";
	if (typecode == ENC_TYPE_WEP)
		return "WEP ";
	if (typecode == ENC_TYPE_TKIP)
		return "WPA ";
	if (typecode == ENC_TYPE_CCMP)
		return "WPA2";
	if (typecode == ENC_TYPE_AUTO)
		return "WPA*";
}

void wifiScan() //扫描周围的wifi信息
{
	String req_json = "";
	Serial.println("Scan WiFi");
	int n = WiFi.scanNetworks();
	if (n > 0)
	{
		int m = 0;
		req_json = "{\"req\":[";
		for (int i = 0; i < n; i++)
		{
			if ((int)WiFi.RSSI(i) >= Signal_filtering)
			{
				m++;
				String a = "{\"ssid\":\"" + (String)WiFi.SSID(i) + "\"," + "\"encryptionType\":\"" + wifi_type(WiFi.encryptionType(i)) + "\"," + "\"rssi\":" + (int)WiFi.RSSI(i) + "},";
				if (a.length() > 15)
					req_json += a;
			}
		}
		req_json.remove(req_json.length() - 1);

		req_json += "]}";
		webServer.send(200, "text/json;charset=UTF-8", req_json);
		Serial.print("Found ");
		Serial.print(m);
		Serial.print(" WiFi!  >");
		Serial.print(Signal_filtering);
		Serial.println("dB");
	}
}

void wifiConfig()
{
	Serial.println("进入wificonfig方法");
	if (webServer.hasArg("ssid") && webServer.hasArg("password") && wifi_flag == "0")
	{
		int ssid_len = webServer.arg("ssid").length();
		int password_len = webServer.arg("password").length();

		if ((ssid_len > 0) && (ssid_len < 33) && (password_len > 7) && (password_len < 65))
		{

			String ssid_str = webServer.arg("ssid");
			String password_str = webServer.arg("password");
			const char *ssid = ssid_str.c_str();
			const char *password = password_str.c_str();
			Serial.print("SSID: ");
			Serial.println(ssid);
			Serial.print("Password: ");
			Serial.println(password);

			Serial.print("Connenting");

			WiFi.begin(ssid, password);
			delay(5000);
			//连接wifi
			unsigned long millis_time = millis();
			while ((WiFi.status() != WL_CONNECTED) && (millis() - millis_time < 8000))
			{
				delay(500);
				Serial.print(".");
			}

			if (WiFi.status() == WL_CONNECTED)
			{

				digitalWrite(LED, HIGH);
				Serial.println("");
				Serial.println("Connected successfully!");
				Serial.print("IP Address: ");
				Serial.println(WiFi.localIP());
				Serial.print("http://");
				Serial.println(Hostname);
				webServer.send(200, "text/plain", "1");
				delay(300);
				wifi_flag = "1";
				wifiname=ssid;
				wifipsw=password;
				Serial.println(wifiname);
				Serial.println(wifipsw);
				write_eeprom(); //调用保存函数
				read_eeprom();
				WiFi.softAPdisconnect();
				delay(1000);
				ESP.restart();
			}
			else
			{
				Serial.println("Connenting failed!");
				webServer.send(200, "text/plain", "0");
			}
		}
		else
		{
			Serial.println("Password format error");
			webServer.send(200, "text/plain", "0");
		}
	}
	else
	{
		Serial.println("Request parameter error");
		webServer.send(200, "text/plain", "0");
	}
}

void initSoftAP(void)
{ //初始化AP模式
	WiFi.mode(WIFI_AP);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	if (WiFi.softAP(AP_NAME))
	{
		Serial.println("ESP8266 SoftAP is right");
	}
}

void initWebServer(void)
{ //初始化WebServer
	Serial.println("启动http服务");
	webServer.on("/", wwwroot);
	if (SPIFFS.exists("/config.html") == 1)
	{
		Serial.println("页面已加载完成");
	}
	webServer.on("/wificonfig", wifiConfig);
	webServer.on("/wifiscan", wifiScan);
	if (wifi_flag == "0")
		webServer.onNotFound([]()
							{ webServer.send(200, "text/html", responseHTML); });
	webServer.begin(); //启动WebServer
}

void initDNS(void)
{ //初始化DNS服务器
	if (dnsServer.start(DNS_PORT, "*", apIP))
	{ //判断将所有地址映射到esp8266的ip上是否成功
		Serial.println("start dnsserver success.");
	}
	else
		Serial.println("start dnsserver failed.");
}

void connectNewWifi(void)
{
	WiFi.hostname(Hostname);   //设置ESP8266设备名
	WiFi.mode(WIFI_STA);	   //切换为STA模式
	WiFi.setAutoConnect(true); //设置自动连接
	read_eeprom();			   //读取wifi信息
	delay(100);
	Serial.print(wifiname);
	Serial.print(wifipsw);
	WiFi.begin(wifiname, wifipsw); //连接上一次连接成功的wifi
	Serial.println("");
	Serial.print("start Connect to wifi");
	int count = 0;

	while (WiFi.status() != WL_CONNECTED)
	{
		digitalWrite(LED, LOW);
		delay(1000);
		digitalWrite(LED, HIGH);
		delay(1000);
		digitalWrite(LED, LOW);
		count++;
		if (count > 15)
		{ //如果8秒内没有连上，就开启Web配网 可适当调整这个时间
			initSoftAP();
			initWebServer();
			initDNS();
			break; //跳出 防止无限初始化
		}
		Serial.print(".");
	}
	Serial.println("");
	if (WiFi.status() == WL_CONNECTED)
	{ //如果连接上 就输出IP信息 防止未连接上break后会误输出
		Serial.println("WIFI Connected!");
		wifi_flag = "1"; //标志为1
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP()); //打印esp8266的IP地址
		webServer.stop();
		digitalWrite(LED, HIGH);
		delay(1000);
	}
}

void wifi_load()
{
	Serial.begin(115200);
	SPIFFS.begin();
	if (SPIFFS.exists("/config.html") == 1)
	{
		Serial.println("存在");
		Serial.println("/config.html");
	}

	pinMode(LED, OUTPUT);
	digitalWrite(LED, LOW);
	connectNewWifi();
}

void wifi_pant()
{
	if (wifi_flag == "0")
	{
		digitalWrite(LED, LOW);
		delay(150);
		digitalWrite(LED, HIGH);
		delay(150);
		digitalWrite(LED, LOW);
		webServer.handleClient();
		dnsServer.processNextRequest();
	}
}

#endif