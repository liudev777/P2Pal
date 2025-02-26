## Build Instructions:
1. git clone the repository
2. At root dir, run the following command to build:
	mkdir build && cd build && cmake .. && cmake --build .
2a. So I coded this project on mac, and it seems that with newer versions of the mac os, there is a problem with the RPATH. I had to run this command in the build dir:
	install_name_tool -add_rpath /path/to/your/Qt/library/directory /path/to/your/executable
On linux you probably don't need it. Let me know if there are issues with this.
3. To run an instance of the P2Pal_Devin_Liu app, you have to find the executable. Mac hides it inside of P2Pal_Devin_Liu.app, but I don't think linux has .app files, so it might be be the ./P2Pal_Devin_Liu executable in the /build dir. Simply run:
	./P2Pal_Devin_Liu
3a. To run the automated test cases, do:
	./P2Pal_Devin_Liu --test
