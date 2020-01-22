from pythonping import ping
import PyQt5

response = ping('8.8.8.8', verbose = False)
for r in response:
	print (r.time_elapsed_ms)