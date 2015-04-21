#include "LeptonThread.h"

#include "Palettes.h"
#include "SPI.h"
#include "Lepton_I2C.h"

#define PACKET_SIZE 164
#define PACKET_SIZE_UINT16 (PACKET_SIZE/2)
#define PACKETS_PER_FRAME 60
#define FRAME_SIZE_UINT16 (PACKET_SIZE_UINT16*PACKETS_PER_FRAME)
#define FPS 27;

LeptonThread::LeptonThread() : QThread()
{
}

LeptonThread::~LeptonThread() {
}
int flag= 0;
void LeptonThread::moveArm(int zone){
    switch(zone){
        case 1:
            printf("Arm movement to zone %d\n",zone);
            break;
        case 2:
            printf("Arm movement to Zone %d\n",zone);
            break;
        case 3:
            printf("Arm movement to Zone %d\n",zone);
            break;
        case 4:
            printf("Arm movement to Zone %d\n",zone);
            break;
        case 5:
            printf("Arm movement to Zone %d\n",zone);
            break;
        case 6:
            printf("Arm movement to Zone %d\n",zone);
            break;
        case 7:
            printf("Arm movement to Zone %d\n",zone);
            break;
        case 8:
            printf("Arm movement to Zone %d\n",zone);
            break;
        case 9:
            printf("Arm movement to Zone %d\n",zone);
            break;
        case 10:
            printf("Arm movement to Zone %d\n",zone);
            break;
        case 11:
            printf("Arm movement to Zone %d\n",zone);
            break;
        case 12:
            printf("Arm movement to Zone %d\n",zone);
            break;
            
    }
}
void LeptonThread::run()
{
	//create the initial image
	myImage = QImage(80, 60, QImage::Format_RGB888);

	//open spi port
	SpiOpenPort(0);

	while(true) {

		//read data packets from lepton over SPI
		int resets = 0;
		for(int j=0;j<PACKETS_PER_FRAME;j++) {
			//if it's a drop packet, reset j to 0, set to -1 so he'll be at 0 again loop
			read(spi_cs0_fd, result+sizeof(uint8_t)*PACKET_SIZE*j, sizeof(uint8_t)*PACKET_SIZE);
			int packetNumber = result[j*PACKET_SIZE+1];
			if(packetNumber != j) {
				j = -1;
				resets += 1;
				usleep(1000);
				//Note: we've selected 750 resets as an arbitrary limit, since there should never be 750 "null" packets between two valid transmissions at the current poll rate
				//By polling faster, developers may easily exceed this count, and the down period between frames may then be flagged as a loss of sync
				if(resets == 750) {
					SpiClosePort(0);
					usleep(750000);
					SpiOpenPort(0);
				}
			}
		}
		if(resets >= 30) {
			qDebug() << "done reading, resets: " << resets;
		}

		frameBuffer = (uint16_t *)result;
		int row, column;
		uint16_t value;
		uint16_t minValue = 65535;
		uint16_t maxValue = 0;

		
		for(int i=0;i<FRAME_SIZE_UINT16;i++) {
			//skip the first 2 uint16_t's of every packet, they're 4 header bytes
			if(i % PACKET_SIZE_UINT16 < 2) {
				continue;
			}
			
			//flip the MSB and LSB at the last second
			int temp = result[i*2];
			result[i*2] = result[i*2+1];
			result[i*2+1] = temp;
			
			value = frameBuffer[i];
			if(value > maxValue) {
				maxValue = value;
			}
			if(value < minValue) {
				minValue = value;
			}
		}

		float diff = maxValue - minValue;
		float scale = 255/diff;
		QRgb color;

        int count [12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		for(int i=0;i<FRAME_SIZE_UINT16;i++) {
			if(i % PACKET_SIZE_UINT16 < 2) {
				continue;
			}
			value = (frameBuffer[i] - minValue) * scale;
         
//            if(value >= 200){
//                x=(i-2)%80;
//                y=(i-2)/80;
//                // printf("Bright pixel is at %d %d\n",(i-2)%80, (i-2)/80);
//                count[x/20+(y/3*3)]++;
//                
//            }
            
			const int *colormap = colormap_grayscale;
			color = qRgb(colormap[3*value], colormap[3*value+1], colormap[3*value+2]);
			column = (i % PACKET_SIZE_UINT16 ) - 2;
			row = i / PACKET_SIZE_UINT16;
            if(value >= 100){
                // printf("Bright pixel is at %d %d\n",(i-2)%80, (i-2)/80);
                count[row/20+(column/3*3)]++;
            }
			myImage.setPixel(column, row, color);
		}
		if(flag == 0){
	        for(int i=0;i<12;i++){
	        	if(count[i]>=5){
	        		flag=1;
	        		printf("Heat Source detected at Zone %d\n",i);
                    moveArm(i);
	        	}
	        	

	        	printf("\nZone %d has %d hot pixels\n", i, count[i]);	
	        	
	            
	        }
	    }

		//lets emit the signal for update
		emit updateImage(myImage);

	}
	
	//finally, close SPI port just bcuz
	SpiClosePort(0);
}



void LeptonThread::performFFC() {
	//perform FFC
	lepton_perform_ffc();
}
