# DoS Attacker
A simple cmd DoS Attacker for Windows that supports TCP and ICMP attacks.

**Please note that most websites have DoS protection. You can use it to attack small websites and local devices. You should'nt harm anyone with this program. It is was made for learning purposes only. I take no responsibility for whatever you decide to do with it.**

## Wait... What is this program good for? You just said that it's worthless!
That's a good question!
Well, you can't crash Google's servers with this (and you should'nt even try!), **but** this program is super effective in small networks. It can slow down (or even disable) the connection of the devices in a private network or cause a device to crash.  
**And always remember, this program wasn't meant to be a weapon. Don't harm people with it.**

## Usage:
*"Dos Attacker.exe" target_ip /mode args packet_size threads delay*

*victim* is the IP address or the domain name of your victim.  
*/mode* is the mode to use: */port* for TCP attack (args is the port) **or** */icmp* for ping attack (args is just nothing).  
*packet_size* is the size of the packet that will be send to the victim each time. This argument can be up to 65535 (and should be less to avoid WSAEMSGSIZE errors).  
*threads* is the number of threads that attack the victim.  
*delay* is the time (in milliseconds) that every thread waits between each package.

**Usgae Examples:**  
~~~
"DoS Attacker.exe" 10.100.102.1 /icmp 8192 100 0

"DoS Attacker.exe" www.example.com /icmp 65507 100 0

"DoS Attacker.exe" 10.100.102.1 /port 80 8195 1000 0

"DoS Attacker.exe" 10.100.102.1 /port http 8195 50 0

"DoS Attacker.exe" www.example.com /port http 32 4096 0
~~~
## How it works:
The program connects to the server, using the mode you chose.  
Next, it creates the threads that attack the victim. Those threads attacks the victim as fast as they can.  

If you want to trick a victim that has a DoS protection, you should use a delay and perform a DDoS attack (attack from multiple IPs) and use many threads.  
Your connection speed is also important for your attack success. Slow connection won't affect the victim.
