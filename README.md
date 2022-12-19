# CN-phase2    
This is for CN phase2
## member
### b09902117 林榮盛
### b09902001 李奕辰
## Compile  
### use "gcc server.c -o server" or "make server"to compile  
## Run  
### use "./server" to run
### There is a IP and Port Num in third line(it may be Private IP, not Public IP.If it is Private IP, then we can connect to webpage via this IP)
### If IP address is not work well, there is another process run in GCP, and it's IP and Port Num is" 34.81.176.7:7891 "(http://34.81.198.89:7891/)
## application
### message board:
### The message board will show the most recent four messages.When triggering the send button, it will send post request to server and record both in message board and database.
### Video streaming:
### Use m3u8 and HLS protocol to transmit and play video.
### Audio streaming:
### Use m3u8 and HLS protocol to transmit and play audio.
