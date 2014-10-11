#include<alsa/asoundlib.h>  // sudo apt-get install libasound2-dev
#include<climits>
#include<iostream>
#include<sstream>
#include<fstream>
#include<string>
#include<vector>
#include<array>
#include<map>
#include<set>
#include<algorithm>
#include<stdexcept>
#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<unistd.h>
#include<sys/ioctl.h>

#include"blit_saw_oscillator.h"

int main()
{
    snd_pcm_t *pcm = nullptr;
    try
    {
        // open audio device
        // BEHRINGER UCA222 ---> http://www.behringer.com/EN/Products/UCA222.aspx
        if( ::snd_pcm_open(
            &pcm,
            "hw:CODEC",
            SND_PCM_STREAM_PLAYBACK,
            0) )
        {
            throw std::runtime_error("snd_pcm_open error");
        }

        // set parameters
        if( ::snd_pcm_set_params(
            pcm,
            SND_PCM_FORMAT_S16,
            SND_PCM_ACCESS_RW_INTERLEAVED,
            1, // mono
            44100,
            0,
            20*1000) ) // 20ms
        {
            throw std::runtime_error("snd_pcm_set_params error");
        }

        // get buffer size and period size
        snd_pcm_uframes_t buffer_size = 0;
        snd_pcm_uframes_t period_size = 0;
        if( ::snd_pcm_get_params(
            pcm,
            &buffer_size,
            &period_size) )
        {
            throw std::runtime_error("snd_pcm_get_params error");
        }

        // allocate buffer
        std::vector<short> buffer(period_size);

        std::cout << "start ("
                  << "buffer_size=" << buffer_size << ", "
                  << "period_size=" << period_size << ")"
                  << std::endl;

        //////////////////////
		unsigned short port = 10000;
		int srcSocket; 

		struct sockaddr_in srcAddr;
		struct sockaddr_in dstAddr;
		socklen_t dstAddrSize = sizeof(dstAddr);

		memset(&srcAddr, 0, sizeof(srcAddr));
		srcAddr.sin_port = htons(port);
		srcAddr.sin_family = AF_INET;
		srcAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		srcSocket = socket(AF_INET, SOCK_DGRAM, 0);

		bind(srcSocket, (struct sockaddr *) &srcAddr, sizeof(srcAddr));

		// non blocking
		int val = 1;
		ioctl(srcSocket, FIONBIO, &val);
		//////////////////////

        // オシレータ作成
        blit_saw_oscillator oscillator(0.995, 44100);

        do{
            //-------------
            // OSC receive
            //-------------
            do{
            	
            	////////////////////
            	unsigned char buf[256] = {};
				int numrcv_all = 0;
				int numrcv = 0;
				do{
					numrcv = recv(srcSocket, (char*)buf, sizeof buf, 0);
					if(numrcv == -1) {
						break;
					}
					numrcv_all += numrcv;
				}while( numrcv_all < 56 );
				
				if(numrcv == -1) {
					break;
				}
				//printf("receive size:%d bytes\n", numrcv_all);
            	
				//for( int ii = 0; ii < numrcv_all/4; ii++ )
				//{
				//	printf("0x%02x%02x%02x%02x ",  buf1[4*ii],  buf1[4*ii+1], buf1[4*ii+2], buf1[4*ii+3]);
				//}
				//printf("\n");
			
				unsigned char* pos = buf;
				
				// OSC-string 
				const unsigned char osc_string[8] = {'#', 'b', 'u', 'n', 'd', 'l', 'e', '\0'};
				if( memcmp(pos, osc_string , sizeof osc_string ) != 0 ){
					return 0;
				}
            	
            	//printf("OSC-string:%s =>OK\n", pos);
            	
            	pos+=sizeof osc_string;
				
				// OSC-timetag
				const unsigned char osc_timetag_a[8] = {pos[7], pos[6], pos[5], pos[4], pos[3], pos[2], pos[1], pos[0]};
            	uint64_t osc_timetag = *(uint64_t*)osc_timetag_a;
				if( osc_timetag != 1 ){
					return 0;
				}
            	
            	//printf("OSC-timetag:%d =>OK\n", 1);
            	
				pos+=sizeof osc_timetag;
				
				// bundle element's size
            	const unsigned char element_size_a[4] = {pos[3], pos[2], pos[1], pos[0]};
            	uint32_t element_size = *(uint32_t*)element_size_a;
				if( element_size != 36UL ){
					return 0;
				}
            	
            	//printf("bundle element's size:%d =>OK\n", element_size);
            	
            	pos+= sizeof element_size_a;
				
				// bundle element's contents
				const unsigned char element_content[16] = {
					'/', 'O', 'S', 'C', '_', 'S', 'e', 'n', 'd', '_', 'N', 'o', 't', 'e', '\0', '\0'};
				if( memcmp(pos, element_content , sizeof element_content ) != 0 ){
					return 0;
				}
            	
            	//printf("bundle element's contents:%s =>OK\n", pos);
				
				pos+=sizeof element_content;
            	
            	// OSC Type Tag String
				const unsigned char format[8] = { ',', 'f', 'f', 'f', '\0', '\0', '\0', '\0'};
				if( memcmp(pos, format , sizeof format ) != 0 ){
					return 0;
				}
            	
            	//printf("OSC-Type-Tag:%s =>OK\n", pos);
				
				pos+=sizeof format;
            	
				const unsigned char velocity_a[] = {pos[3], pos[2], pos[1], pos[0]};
				float velocity = *(float*)(velocity_a);
            	pos+=sizeof(float);
				
				const unsigned char pitch_a[] = {pos[3], pos[2], pos[1], pos[0]};
				float pitch = *(float*)(pitch_a);
            	pos+=sizeof(float);

            	const unsigned char gate_a[] = {pos[3], pos[2], pos[1], pos[0]};
				float gate = *(float*)(gate_a);
				pos+=sizeof(float);
				
				::printf("velocity:%f, pitch:%f, gate:%f\n",velocity, pitch, gate);
	            ::fflush(stdout);

                if( gate != 0.0){
                    oscillator.trigger(int(pitch+0.5), int(velocity*127+0.5));
                }
                else{
                    oscillator.release(int(pitch+0.5));
                }
            }while(true);

            //---------------
            // audio process
            //---------------
            for(short& s : buffer)
            {
                s = 1000.0*oscillator.render();
                oscillator.next();
            }

            //-------------
            // write audio
            //-------------
            if( ::snd_pcm_writei(pcm, buffer.data(), period_size) < 0 )
            {
                throw std::runtime_error("snd_pcm_writei error");
            }
        }while(true);
    	close(srcSocket);

        std::cout << std::endl << "end" << std::endl;
    }
    catch(std::exception &ex)
    {
        std::cout << std::endl << "error:" << ex.what() << std::endl;
    }

    if(pcm != nullptr)
    {
        // close audio device
        ::snd_pcm_close(pcm);
    }

    return 0;
}
