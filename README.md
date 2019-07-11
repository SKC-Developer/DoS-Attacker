# DoS Attacker
A simple cmd DoS Attacker for Windows.

**Please note that most websites have DoS protection. You can use it to attack small websites and access points. Use it at your own risk.**
## Usage:
*"Dos Attacker.exe" target_ip /mode args package_size threads delay*

*target_ip* is the IP address or the domain name of your victim.  
*/mode* is the mode to use: */port* for TCP attack (args is the port) **or** */icmp* for ping attack (args is just nothing).  
*package_size* is the size of the package that will be send to the victim each time. This size is limited according to the socket type: 65467 for */icmp* and 65536 for */port*.  
*threads* is the number of threads that attack the victim.  
*delay* is the time (in milliseconds) that every thread waits between each package.

**Examples:**  
*"DoS Attacker.exe" 10.100.102.1 /icmp 8192 100 0*  

*"DoS Attacker.exe" www.example.com /icmp 65536 100 0*  

*"DoS Attacker.exe" 10.100.102.1 /port 80 8195 1000 0*

*"DoS Attacker.exe" 10.100.102.1 /port http 8195 50 0*

*"DoS Attacker.exe" www.example.com /port http 32 4096 0*

## How it works:
The program connects to the server, using the mode you chose.  
Next, it creates the threads that attack the victim. Those threads attacks the victim as fast as they can.  

If you want to trick a victim that has a DoS protection, you should use a delay and perform a DDoS attack (attack from multiple IPs) and use many threads.  
Your connection speed is also important for your attack success. Slow connection won't affect the victim.
