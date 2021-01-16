#include "NtpServer.h"

//init Ntp class with SSID and Password
NtpServer::NtpServer(char *ssid, char *pwd)
{
    strcpy(_ssid, ssid);
    strcpy(_pwd, pwd);
}
//connect WIFI AP
void NtpServer::connectToWiFi()
{
    Serial.println("Connecting to WiFi network: " + String(_ssid));

    // delete old config
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);

    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
    {
        Serial.println("no networks found");
    }
    else
    {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i)
        {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
        }
    }
    Serial.println("Waiting for WIFI connection...");
    Serial.println(_ssid);
    Serial.println(_pwd);

    //Initiate connection
    WiFi.begin(_ssid, _pwd);

    while (WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin(_ssid, _pwd);
        delay(500);
    }
    Serial.println("Connected.");
}
//get current time from NTP server
unsigned long NtpServer::getNTPtime()
{
    connectToWiFi();
    if (WiFi.status() == WL_CONNECTED)
    {
        //initializes the UDP state
        //This initializes the transfer buffer

        udp.begin(WiFi.localIP(), localPort);

        sendNTPpacket(timeServer); // send an NTP packet to a time server
        // wait to see if a reply is available
        delay(1000);

        if (udp.parsePacket())
        {
            Serial.println("udp packet received");
            Serial.println("");
            // We've received a packet, read the data from it
            udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

            //the timestamp starts at byte 40 of the received packet and is four bytes,
            // or two words, long. First, extract the two words:

            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
            // combine the four bytes (two words) into a long integer
            // this is NTP time (seconds since Jan 1 1900):
            unsigned long secsSince1900 = highWord << 16 | lowWord;
            // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
            const unsigned long seventyYears = 2208988800UL;
            // subtract seventy years:
            unsigned long epoch = secsSince1900 - seventyYears;

            // adjust time for timezone offset in secs +/- from UTC
            // WA time offset from UTC is +8 hours (28,800 secs)
            // + East of GMT
            // - West of GMT
            long tzOffset = 28800UL;

            // WA local time
            unsigned long adjustedTime;
            return adjustedTime = epoch + tzOffset;
        }
        else
        {
            // were not able to parse the udp packet successfully
            // clear down the udp connection
            udp.stop();
            return 0; // zero indicates a failure
        }
        // not calling ntp time frequently, stop releases resources
        udp.stop();
    }
    else
    {
        // network not connected
        return 0;
    }
}

unsigned long NtpServer::sendNTPpacket(const char *address)
{
    // set all bytes in the buffer to 0
    for (int i = 0; i < NTP_PACKET_SIZE; ++i)
    {
        packetBuffer[i] = 0;
    }
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
}

