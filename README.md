# DoS Attacker
A simple cmd DoS Attacker for Windows.

**Please note that most websites have DoS protection. You can use it to attack small websites and access points. Use it at your own risk.**
## Usage:
*"Dos Attacker.exe" target_ip service_name package_size delay*

*target_ip* is the IP address of your victim. You can get it using *ping www.example.com -a*.  
*service_name* is the name of the service or the port number which you want to use to attack the victim.
*package_size* is the size of the package that will be send to the victim each time.  
*delay* is the time (in milliseconds) that every thread waits between each package.

**Example:**  
*"DoS Attacker.exe" 10.100.102.1 http 8195 0*  
The program will attack 10.100.102.1:80 with package size of 8195 and no delay.

This example works too: *"DoS Attacker.exe" 10.100.102.1 80 8195 0*

**Important:**

package_size can't be more than SO_MAX_MSG_SIZE (it's usually 8195).  
You can get the list of services from *'%WINDIR%\\system32\\drivers\\etc\\services'*.  
You can use *Port Scanner* (https://github.com/SKC-Developer/Port-Scanner.git) to get a list of all the open ports of your victim.

## How it works:
The program connects to the server, using the service you have chose.  
Next, it creates a thread that attack the victim. This thread attacks the victim as fast as it can.  
If any error occors (let's say, the victim closes the connection) the thread stops and the program closes.

If you want to trick a victim with DoS protection, you should use a delay and perform a DDoS attack (attack from multiple PCs).  
Your connection speed is also important for your attack success. Slow connection won't affect the victim.
