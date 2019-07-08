# DoS Attacker
A simple cmd DoS Attacker for Windows.

**Please note that most websites have DoS protection. You can use it to attack small websites and access points. Use it at your own risk.**
## Usage:
*"Dos Attacker.exe" target_ip /mode args package_size delay*

*target_ip* is the IP address of your victim. You can get it using *ping www.example.com -a*.  
*/mode* is the mode to use: */port* for TCP attack (args is the port) **or** */raw* for raw attack (args is NULL).  
*package_size* is the size of the package that will be send to the victim each time.  
*delay* is the time (in milliseconds) that every thread waits between each package.

**Examples:**  
*"DoS Attacker.exe" 10.100.102.1 /raw 8195 0*  

*"DoS Attacker.exe" www.example.com /raw 8195 0*  

*"DoS Attacker.exe" 10.100.102.1 /port 80 8195 0*

*"DoS Attacker.exe" 10.100.102.1 /port http 8195 0*

*"DoS Attacker.exe" www.example.com /port http 8195 0*

**Important:**

package_size can't be more than SO_MAX_MSG_SIZE (it's usually 8195).  

## How it works:
The program connects to the server, using the mode you chose.  
Next, it creates a thread that attack the victim. This thread attacks the victim as fast as it can.  
If any error occors (let's say, the victim closes the connection) the thread stops and the program closes.

If you want to trick a victim that has a DoS protection, you should use a delay and perform a DDoS attack (attack from multiple IPs).  
Your connection speed is also important for your attack success. Slow connection won't affect the victim.
