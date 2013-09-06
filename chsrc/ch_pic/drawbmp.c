#include "graphcom.h"
#include "parsebmp.h"


BMPImage Image;
#ifdef USE_ARGB8888	
typedef struct bitadd
		{
			U8	LowAdd:4;
			U8	HighAdd:4;
		} bbit4;


	
#endif



U32 RealWidth = 0;/*20080222 move */
void UnpacketDiff(MYBITMAPINFOHEADER* differentdata,graph_file* filepoint,U8* picdata,U32 realwidth)/* 20080222 add */
{
	if(differentdata == NULL || filepoint == NULL || picdata == NULL)
		return;
	differentdata->biCompression = BI_RGB;
	filepoint->tmpbuf = picdata;
	differentdata->biSizeImage = 0x00;
	differentdata->bfSize = realwidth * differentdata->biHeight + differentdata->bfOffBits;	
}

PCH_RECT CH_GetBmpSize(S8* FileName, int FileSize)
{
	PCH_RECT bmpsize = NULL;
	MYBITMAPINFOHEADER BmpInfoHeader;
	graph_file* pFile = graph_open((char*)FileName, "rb",FileSize);

	if(!pFile)
	{
#ifdef ENABLE_CH_PRINT	
		CH_Printf("Read Bmp File Open Error\n");
#endif
		return NULL;
	}	
	
	graph_read(&BmpInfoHeader.bfType,2,1,pFile);
	graph_read(&BmpInfoHeader.bfSize,4,1,pFile);
	graph_read(&BmpInfoHeader.bfReserved1,2,1,pFile);
	graph_read(&BmpInfoHeader.bfReserved2,2,1,pFile);
	graph_read(&BmpInfoHeader.bfOffBits,4,1,pFile);
	graph_read(&BmpInfoHeader.biSize,4,1,pFile);
	graph_read(&BmpInfoHeader.biWidth,4,1,pFile);
	graph_read(&BmpInfoHeader.biHeight,4,1,pFile);
	graph_close(pFile);
	bmpsize = (PCH_RECT)CH_AllocMem(sizeof(CH_RECT));
	if(!bmpsize)
	{
#ifdef ENABLE_CH_PRINT	
		CH_Printf("Drawbmp.c @ alloc memory fail\n");
#endif
		return NULL;
	}
	bmpsize->iStartX = 0;
	bmpsize->iStartY = 0;
	bmpsize->iWidth = (S16)BmpInfoHeader.biWidth;
	bmpsize->iHeigh = (S16)BmpInfoHeader.biHeight;
	return bmpsize;
}
/*#define dz_DEBUG_BMP*/
CH_STATUS CH_DrawBmp(U8* FileName,PCH_WINDOW ch_this,PCH_RECT RectSize, id_backgroundRepeat repeat,int ri_len)
{
	S16 width = 0, depth = 0;
	S16 lNumRead = 0;
	S16 i = 0,j = 0, row = 0;
	S16 BmpStart = 0;
	U32 BmpSize = 0, widthlen= 0;
	U32 BmpIndexSize = 0;
	U32 iSize = 0;
	U8 *pData = NULL, *tmp = NULL, *buf = NULL, *buffer = NULL;
	S16 Colors = 0;
	U32 *Palettes = NULL;
	U8 Index = 0;
	S16 num=0;
	U8 alpha = 0;
	CH_RGB_COLOUR struct_color;
	CH_POINT point;
	/*BMPImage BmpImage;*/
	MYBITMAPINFOHEADER BmpInfoHeader;
	CH_RECT bmprect;
	U8* UnCommpressData = NULL;/* 20080219 add */
	U8* UnpacketData = NULL;
	U8* FreeDataAdd =NULL;
	boolean IsPacket = false;
	boolean ReverseType = false;
	unsigned char* CurSourceTempAdd = NULL;
	int l = 0;
	graph_file* pFile = graph_open((char*)FileName, "rb",ri_len);
	
	#ifdef dz_DEBUG_BMP
	STTBX_Print("start draw bmp\n");
	#endif
	if(!pFile)
	{
	#ifdef ENABLE_CH_PRINT
		CH_Printf("Read Bmp File Open Error\n");
	#endif
		return CH_ERROR_PARAM;
	}
#ifdef ENABLE_CH_PRINT	
	else
	{
		CH_Printf("open bmp file (obj = %d) ok\n", FileName);
	}
#endif	
	CH_GET_RGB_COLOR((&struct_color)).uAlpha = 0xff;
	graph_read(&BmpInfoHeader.bfType,2,1,pFile);
	graph_read(&BmpInfoHeader.bfSize,4,1,pFile);
	graph_read(&BmpInfoHeader.bfReserved1,2,1,pFile);
	graph_read(&BmpInfoHeader.bfReserved2,2,1,pFile);
	graph_read(&BmpInfoHeader.bfOffBits,4,1,pFile);
	graph_read(&BmpInfoHeader.biSize,4,1,pFile);
	graph_read(&BmpInfoHeader.biWidth,4,1,pFile);
	graph_read(&BmpInfoHeader.biHeight,4,1,pFile);
	graph_read(&BmpInfoHeader.biPlanes,2,1,pFile);
	graph_read(&BmpInfoHeader.biBitCount,2,1,pFile);
	graph_read(&BmpInfoHeader.biCompression,4,1,pFile);
	graph_read(&BmpInfoHeader.biSizeImage,4,1,pFile);
	graph_read(&BmpInfoHeader.biXPelsPerMeter,4,1,pFile);
	graph_read(&BmpInfoHeader.biYPelsPerMeter,4,1,pFile);
	graph_read(&BmpInfoHeader.biClrUsed,4,1,pFile);
	graph_read(&BmpInfoHeader.biClrImportant,4,1,pFile);
	
	if(BmpInfoHeader.biHeight != abs((S16)BmpInfoHeader.biHeight))
	{
			BmpInfoHeader.biHeight = abs((S16)BmpInfoHeader.biHeight);
			ReverseType = true;	
	}

	
#ifdef ENABLE_CH_PRINT	
	CH_Printf("drawbmp.c : picture heigh = %d, width = %d\n", BmpInfoHeader.biHeight, BmpInfoHeader.biWidth);
#endif
	BmpStart = (S16)BmpInfoHeader.bfOffBits;
	BmpSize = (U32)(BmpInfoHeader.biWidth*BmpInfoHeader.biHeight);
	BmpIndexSize = BmpInfoHeader.bfSize-BmpStart;
	point.y = RectSize->iStartY + (S16)BmpInfoHeader.biHeight - 1;
	point.x = RectSize->iStartX;
	switch(BmpInfoHeader.biBitCount)
		{
			case 1:
			if(BmpInfoHeader.biCompression == BI_RGB)
				{
					
					if(BmpInfoHeader.biClrImportant)
						Colors = BmpInfoHeader.biClrImportant;
					else if(BmpInfoHeader.biClrUsed)
						Colors = BmpInfoHeader.biClrUsed;
					else 
						Colors = 2;

					Palettes = (U32 *)CH_AllocMem(sizeof(U32)*Colors);
					if(!Palettes)
					{
				#ifdef ENABLE_CH_PRINT	
						CH_Printf("drawbmp.c @ alloc error!\n");
				#endif
						return CH_ALLOCMEM_ERROR;
					}
					
					graph_read(Palettes,sizeof(U32),Colors,pFile);


					graph_seek(pFile,BmpStart,0);
					j = 0;
					
					if(BmpInfoHeader.biWidth%32==0)	 
						width=BmpInfoHeader.biWidth;	
					else	
						width=BmpInfoHeader.biWidth+32-BmpInfoHeader.biWidth%32;

					
				if(ReverseType)
					{
						buffer = (U8*)(ch_this->scrBuffer.dwHandle		\
								+ RectSize->iStartY  * ch_this->SingleScrWidth * ch_this->ColorByte		\
								+ RectSize->iStartX * ch_this->ColorByte);
					}
				else
					{
						buffer = (U8*)(ch_this->scrBuffer.dwHandle		\
								+ (RectSize->iStartY + (BmpInfoHeader.biHeight-1)) * ch_this->SingleScrWidth * ch_this->ColorByte		\
								+ RectSize->iStartX * ch_this->ColorByte);
					}
						while(j < BmpInfoHeader.biHeight)
						{
							j ++;
							buf = buffer;l = 0;
							for(i=0;i<width/8;i++)
							{
									int k=0;
								graph_read(&Index,1,1,pFile);
									while ((k<8)&&(l < BmpInfoHeader.biWidth))
#ifdef USE_ARGB8888
									{
									        U8 u_Index = (Index>>(7-k))&0x01;
											*(buf + 4*k) = Palettes[u_Index]&0xFF;		/*b*/
											*(buf + 4*k+1) = (Palettes[u_Index]>>8)&0xFF;	/*g*/
											*(buf + 4*k+2) = (Palettes[u_Index]>>16)&0xFF;	/*r*/
											*(buf + 4*k+3) = 0xff;					/*a*/
											/*Index = Index/2;*/
											k++;
											l++;
									}
									buf+=32;
#elif USE_ARGB1555
									{
									      U8 u_Index = (Index>>(7-k))&0x01;
											*(U16*)(buf + 2*k)= 0x8000 |((((Palettes[u_Index]>>16) & 0x00ff)>>3)<<10 ) | ((((Palettes[u_Index]>>8)& 0x00ff)>>3)<<5) | ((Palettes[u_Index] & 0x00ff)>>3);   /*b*/
											/*Index = Index/2;*/
											k++;
											l++;
									}
									buf+=16;
#elif USE_ARGB4444			
                                    {
									      U8 u_Index = (Index>>(7-k))&0x01;
											*(U16*)(buf + 2*k)= 0xf000 |((((Palettes[u_Index]>>16) & 0x00ff)>>4)<<8) | ((((Palettes[u_Index]>>8)& 0x00ff)>>4)<<4) | ((Palettes[u_Index] & 0x00ff)>>4);   /*b*/
											/*Index = Index/2;*/ 
											k++;
											l++;
									}
									buf+=16;
#endif
							}
					if(ReverseType)
							buffer+=ch_this->SingleScrWidth * ch_this->ColorByte;
					else
						buffer -= ch_this->SingleScrWidth * ch_this->ColorByte;
						}
				
				graph_close(pFile);
					}
					else
					{
							graph_close(pFile);
				return CH_FAILURE;
			}
			
				break;
			case 4:
				if(/*BmpInfoHeader.biCompression == BI_RLE4*/BmpInfoHeader.biCompression !=BI_RGB)
					{
						boolean IsODDNumber = false;
					#ifdef USE_ARGB8888	
						bbit4 *bit4 = NULL;
					#else
					   U8 *bit4 = NULL;

					#endif
						U8*	AlignSignal = NULL;
						U8* EndAddress = NULL;
						IsPacket = true;
						/*U32*/	RealWidth = (BmpInfoHeader.biWidth * 4 + 31)/32 * 4;

#if 0
						if(BmpInfoHeader.biWidth%8==0)	 
											RealWidth=BmpInfoHeader.biWidth;	
										else	
											width=BmpInfoHeader.biWidth+8-BmpInfoHeader.biWidth%8;
#endif
						
						UnCommpressData = (U8*)CH_AllocMem(RealWidth *BmpInfoHeader.biHeight + 2);	
						memset(UnCommpressData,0x0,RealWidth *BmpInfoHeader.biHeight + 2);
						FreeDataAdd = UnpacketData = AlignSignal = UnCommpressData;
						EndAddress = UnCommpressData +  RealWidth *BmpInfoHeader.biHeight;
						if(!UnCommpressData)
							{
								do_report(0,"Not enough space for bmp unpacket(bit4)!\n");
								return CH_FAILURE;
							}
						CurSourceTempAdd = pFile->linked_obj_addr + BmpStart;
						while(UnCommpressData < EndAddress)
							{
								if(CurSourceTempAdd[0] == 0x00)
									{
										if(CurSourceTempAdd[1] == 0x00 || CurSourceTempAdd[1] == 0x01 || CurSourceTempAdd[1] == 0x02)
											{
												boolean ErrFlag =false;
												U16 AddNullCount = RealWidth - (UnCommpressData - AlignSignal);
												if(UnCommpressData - AlignSignal > RealWidth)
													{
														ErrFlag = true;
														AddNullCount = UnCommpressData - AlignSignal -RealWidth;
													}
												else
													AddNullCount = RealWidth - (UnCommpressData - AlignSignal) ;
												
												if(CurSourceTempAdd[1] == 0x02)
													{
														U16 AlignTemp =UnCommpressData - AlignSignal;
														if(IsODDNumber == false)
															{
																if(CurSourceTempAdd[2] % 2 == 0)
																	{
																		UnCommpressData += RealWidth * CurSourceTempAdd[3] + CurSourceTempAdd[2]/2;
																		if(CurSourceTempAdd[3] > 0 || (CurSourceTempAdd[2]/2 + AlignTemp) > RealWidth)
																			AlignSignal = UnCommpressData - (((CurSourceTempAdd[2]/2 + AlignTemp) > RealWidth) ? (CurSourceTempAdd[2]/2 + AlignTemp - RealWidth) : (AlignTemp + CurSourceTempAdd[2]/2));
																	}
																else
																	{
																		UnCommpressData += RealWidth * CurSourceTempAdd[3] + CurSourceTempAdd[2]/2 + 1;
																		if(CurSourceTempAdd[3] > 0  || (CurSourceTempAdd[2]/2 + AlignTemp) > RealWidth)
																			AlignSignal = UnCommpressData - (((CurSourceTempAdd[2]/2 + 1 + AlignTemp) > RealWidth) ? (CurSourceTempAdd[2]/2 + AlignTemp - RealWidth) : (AlignTemp + CurSourceTempAdd[2]/2 + 1));
																		IsODDNumber = true;
																	}																	
															}
														else
															{
																if(CurSourceTempAdd[2] % 2 != 0)
																	{
																		UnCommpressData += RealWidth * CurSourceTempAdd[3] + CurSourceTempAdd[2]/2;
																		if(CurSourceTempAdd[3] > 0 || (CurSourceTempAdd[2]/2 + AlignTemp) > RealWidth)
																			AlignSignal = UnCommpressData - (((CurSourceTempAdd[2]/2 + AlignTemp) > RealWidth) ? (CurSourceTempAdd[2]/2 + AlignTemp - RealWidth) : (AlignTemp + CurSourceTempAdd[2]/2));
																		IsODDNumber = false;
																	}
																else
																	{
																		UnCommpressData += RealWidth * CurSourceTempAdd[3] + CurSourceTempAdd[2]/2;
																		if(CurSourceTempAdd[3] > 0 || (CurSourceTempAdd[2]/2 + AlignTemp) > RealWidth)
																			AlignSignal = UnCommpressData - (((CurSourceTempAdd[2]/2 + AlignTemp) > RealWidth) ? (CurSourceTempAdd[2]/2 + AlignTemp - RealWidth) : (AlignTemp + CurSourceTempAdd[2]/2));
																	}														
															}
														
														CurSourceTempAdd += 4;
													}
												else if(CurSourceTempAdd[1] == 0x01)
													{
														memset(UnCommpressData,0x00,2);
														break;
													}
												else
													{
													
														if(ErrFlag == true)
															{
																ErrFlag == true;
																CurSourceTempAdd +=2;
																UnCommpressData -= AddNullCount;
																AlignSignal = UnCommpressData;
																IsODDNumber = false;
															}
														else if(AddNullCount == 0)
															{
																CurSourceTempAdd +=2;
																AlignSignal = UnCommpressData;
																IsODDNumber = false;
															}
														else if(AddNullCount > 0)
															{
																if(IsODDNumber == true)
																	{
																	
#ifdef USE_ARGB8888	
																		bit4 = UnCommpressData -1;
																		bit4->LowAdd = 0x01 >> 4;
#else
																	bit4 = UnCommpressData -1;	
                                                                    *bit4 = *bit4 &0xF0;
#endif
																	
																	}
																memset(UnCommpressData,0x00,AddNullCount);
																CurSourceTempAdd +=2;
																UnCommpressData = UnCommpressData + AddNullCount;
																AlignSignal = UnCommpressData;
																IsODDNumber = false;
															}
													}
											}
										else
											{
												U8  Loopi;
												if(IsODDNumber == true)
													{
														U16 MoveLENData = (CurSourceTempAdd[1] + 1)/2;
														IsODDNumber = (CurSourceTempAdd[1] % 2 == 0  ? true : false);
#ifdef USE_ARGB8888	
														bit4 = UnCommpressData -1;
														bit4->LowAdd = CurSourceTempAdd[2]>>4;

#else
														bit4 = UnCommpressData -1;
                                                        *bit4 =( *bit4&0xF0)|( (CurSourceTempAdd[2]>>4)&0x0F);
                                                        
#endif
														for(Loopi = 0;Loopi < CurSourceTempAdd[1]/2;Loopi++)
															{
#ifdef USE_ARGB8888	
																bit4 = UnCommpressData+1+Loopi;
																bit4->HighAdd = (CurSourceTempAdd[2 + Loopi] << 4) >> 4;
																bit4->LowAdd = CurSourceTempAdd[3 + Loopi] >> 4;

#else
																bit4 =UnCommpressData+1+Loopi;
                                                                *bit4 =  ((CurSourceTempAdd[2 + Loopi] << 4)&0xF0)|(( CurSourceTempAdd[3 + Loopi] >> 4)&0x0F);
                                                                
#endif

															}
														CurSourceTempAdd += (MoveLENData % 2 == 0) ? (MoveLENData + 2) :(MoveLENData + 1 +2);
														UnCommpressData = UnCommpressData  + ((IsODDNumber == false) ? (MoveLENData - 1) : MoveLENData);
													}
												else
													{
														U16 MoveLENData = (CurSourceTempAdd[1] + 1)/2;
														IsODDNumber = (CurSourceTempAdd[1] % 2 == 0  ? false : true);
														for(Loopi = 0;Loopi < MoveLENData;Loopi++)
															{
#ifdef USE_ARGB8888	
																bit4 = UnCommpressData + Loopi;
																bit4->HighAdd = CurSourceTempAdd[2 + Loopi] >> 4;
																bit4->LowAdd = (CurSourceTempAdd[2+ Loopi] << 4) >> 4;

#else
																bit4 = UnCommpressData + Loopi;
                                                                *bit4 =CurSourceTempAdd[2+ Loopi];
                                                                
#endif

															}
														CurSourceTempAdd += (MoveLENData % 2 == 0) ? (MoveLENData + 2) :(MoveLENData + 1 +2);
														UnCommpressData = UnCommpressData  + MoveLENData;
													}
												
												
												
											}
									}
								else
									{
										if(IsODDNumber == false)
											{
												if(CurSourceTempAdd[0] % 2 == 0)
													{
														memset(UnCommpressData,CurSourceTempAdd[1],CurSourceTempAdd[0]/2);														
														UnCommpressData = UnCommpressData + CurSourceTempAdd[0]/2;
														CurSourceTempAdd += 2;
													}
												else
													{
														memset(UnCommpressData,CurSourceTempAdd[1],(CurSourceTempAdd[0] + 1)/ 2);														
														UnCommpressData = UnCommpressData + (CurSourceTempAdd[0] + 1 )/ 2;
														IsODDNumber = true;
														CurSourceTempAdd += 2;
													}
											}
										else
											{
												U8 RecycleCount = (CurSourceTempAdd[0] +1 )/2;
												if(CurSourceTempAdd[0] % 2 == 0)
													{
														U8 Loopi;
#ifdef USE_ARGB8888	
														bit4 = UnCommpressData -1;
														bit4->LowAdd = CurSourceTempAdd[1] >> 4;

#else
														bit4 = UnCommpressData -1;
                                                        *bit4 =(*bit4&0xF0)|((CurSourceTempAdd[1] >> 4)&0x0F);
#endif
														for(Loopi = 0;Loopi < CurSourceTempAdd[0]/2;Loopi++)
															{
#ifdef USE_ARGB8888	
																bit4 = UnCommpressData + 1 +Loopi;
																bit4->HighAdd = (CurSourceTempAdd[1] << 4) >> 4;
																bit4->LowAdd = CurSourceTempAdd[1] >> 4;

#else
																bit4 = UnCommpressData + 1 +Loopi;
                                                                *bit4 =  ( (CurSourceTempAdd[1] << 4)&0xF0)|((CurSourceTempAdd[1] >> 4)&0x0F);
#endif

															}														
														CurSourceTempAdd += 2;
														UnCommpressData = UnCommpressData + RecycleCount;
													}
												else
													{
														U8 Loopi;
#ifdef USE_ARGB8888	
																bit4 = UnCommpressData -1;
														bit4->LowAdd = CurSourceTempAdd[1] >> 4;

#else
																bit4 = UnCommpressData -1;
                                                               
															   *bit4 =(*bit4&0xF0)|( (CurSourceTempAdd[1] >> 4)&0x0F);
                                                               
#endif

														for(Loopi = 0;Loopi < CurSourceTempAdd[0]/2;Loopi++)
															{
#ifdef USE_ARGB8888	
																bit4 = UnCommpressData + 1 +Loopi;
																bit4->HighAdd = (CurSourceTempAdd[1] << 4) >> 4;
																bit4->LowAdd = CurSourceTempAdd[1] >> 4;

#elif USE_ARGB1555	
																bit4 = UnCommpressData + 1 +Loopi;                                                               
															   *bit4 =	( (CurSourceTempAdd[1] << 4)&0xF0)|((CurSourceTempAdd[1] >> 4)&0x0F);
#elif USE_ARGB4444 
                                                               	bit4 = UnCommpressData + 1 +Loopi;                                                               
															   *bit4 =	( (CurSourceTempAdd[1] << 4)&0xF0)|((CurSourceTempAdd[1] >> 4)&0x0F);
#endif

															}														
														CurSourceTempAdd += 2;
														UnCommpressData = UnCommpressData + RecycleCount - 1;															
														IsODDNumber = false;
													}

											}
									}
							}

					}
				
					
				{
					
					if(BmpInfoHeader.biClrImportant)
						Colors = (S16)BmpInfoHeader.biClrImportant;
					else if(BmpInfoHeader.biClrUsed)
						Colors = (S16)BmpInfoHeader.biClrUsed;
					else 
						Colors = 16;
					Palettes = (U32 *)CH_AllocMem(sizeof(U32)*Colors);
					if(!Palettes)
					{
				#ifdef ENABLE_CH_PRINT	
						CH_Printf("drawbmp.c @ alloc error!\n");
				#endif
						return CH_ALLOCMEM_ERROR;
					}
					
					graph_read(Palettes,sizeof(U32),Colors,pFile);

					graph_seek(pFile,BmpStart,0);
					j = 0;

					
					if(BmpInfoHeader.biWidth%8==0)	 
						width=BmpInfoHeader.biWidth;	
					else	
						width=BmpInfoHeader.biWidth+8-BmpInfoHeader.biWidth%8;
					if(ReverseType)
						point.y = RectSize->iStartY;
						while(j < BmpInfoHeader.biHeight)
						{
							j ++;
							for(i=0;i<width/2;i++)
							{
					 if (IsPacket == true)
					 	{
					 	  Index = *UnpacketData;
						  UnpacketData++;
					 	}
					 
					 	else
								graph_read(&Index,1,1,pFile);
								if((point.x - RectSize->iStartX) < BmpInfoHeader.biWidth)
								{
								CH_GET_RGB_COLOR((&struct_color)).uRed = (U8)((Palettes[(Index&0xf0)>>4]>>16)&0xff);
								CH_GET_RGB_COLOR((&struct_color)).uGreen = (U8)((Palettes[(Index&0xf0)>>4]>>8)&0xff);
								CH_GET_RGB_COLOR((&struct_color)).uBlue = (U8)((Palettes[(Index&0xf0)>>4])&0xff);
								CH_DrawColorPoint(ch_this, &point, &struct_color);
								}
								point.x ++;
								
								if((point.x - RectSize->iStartX) < BmpInfoHeader.biWidth)
									{
								CH_GET_RGB_COLOR((&struct_color)).uRed = (U8)((Palettes[Index&0x0f]>>16)&0xff);
								CH_GET_RGB_COLOR((&struct_color)).uGreen = (U8)((Palettes[Index&0x0f]>>8)&0xff);
								CH_GET_RGB_COLOR((&struct_color)).uBlue = (U8)((Palettes[Index&0x0f])&0xff);
								CH_DrawColorPoint(ch_this, &point, &struct_color);
								}
								point.x ++;
							}	
						
						if(ReverseType)
							point.y ++;
						else
						point.y --; 
							point.x = RectSize->iStartX;
						}
					graph_close(pFile);
					}
				break;
			case 8:
			if(/*BmpInfoHeader.biCompression == BI_RLE8*/BmpInfoHeader.biCompression != BI_RGB)/* 20080222 add */
			{
				U8* AlignSignal = NULL;
				U8* EndAddress = NULL;
				IsPacket = true;
				/*U32 */RealWidth = (BmpInfoHeader.biWidth * 8 + 31)/32 * 4;
				UnCommpressData = (U8*)CH_AllocMem(RealWidth *BmpInfoHeader.biHeight + 2);	
				memset(UnCommpressData,0x0,RealWidth *BmpInfoHeader.biHeight + 2);
				FreeDataAdd = UnpacketData = AlignSignal = UnCommpressData;
				EndAddress = UnCommpressData +	RealWidth *BmpInfoHeader.biHeight;
				if(!UnCommpressData)
					{
						do_report(0,"Not enough space for bmp unpacket(bit8)!\n");
						return CH_FAILURE;
					}
				CurSourceTempAdd = pFile->linked_obj_addr + BmpStart;
				while(UnCommpressData < EndAddress)
					{
						if(CurSourceTempAdd[0] == 0x00)
							{
								if(CurSourceTempAdd[1] == 0x00 || CurSourceTempAdd[1] == 0x01 || CurSourceTempAdd[1] == 0x02)
									{
										U16 AddNullCount = RealWidth - (UnCommpressData - AlignSignal);
										
										if(CurSourceTempAdd[1] == 0x02)
											{
												U16 AlignTemp =UnCommpressData - AlignSignal;
												UnCommpressData += RealWidth * CurSourceTempAdd[3] + CurSourceTempAdd[2];
												if(CurSourceTempAdd[3] > 0 || (CurSourceTempAdd[2] + AlignTemp) > RealWidth)
													{
														AlignSignal = UnCommpressData - (((CurSourceTempAdd[2] + AlignTemp) > RealWidth) ? (CurSourceTempAdd[2] + AlignTemp - RealWidth) : (AlignTemp + CurSourceTempAdd[2]));
													}
												CurSourceTempAdd += 4;
											}
										else if(CurSourceTempAdd[1] == 0x01)
											{
												memset(UnCommpressData,0x00,2);
												break;
											}
										else
											{
												if(AddNullCount == 0)
													{
														CurSourceTempAdd +=2;
														AlignSignal = UnCommpressData;
													}
												else if(AddNullCount > 0)
													{
														memset(UnCommpressData,0x00,AddNullCount);
														CurSourceTempAdd +=2;
														UnCommpressData = UnCommpressData + AddNullCount;
														AlignSignal = UnCommpressData;
													}
												else
													{
														CurSourceTempAdd +=2;
														UnCommpressData -= abs(AddNullCount);
														AlignSignal = UnCommpressData;
													}
											}
									}
								
								else
									{
										U16 CopyLEN = CurSourceTempAdd[1];
										memcpy(UnCommpressData,&CurSourceTempAdd[2],CopyLEN);
										
										CurSourceTempAdd += CopyLEN % 2 == 0 ? CopyLEN + 2 :CopyLEN + 1 +2;
										UnCommpressData = UnCommpressData  + CopyLEN;
										
									}
							}
						else
							{
								U16 RecycleCount = CurSourceTempAdd[0];
								memset(UnCommpressData,CurSourceTempAdd[1],RecycleCount);
								CurSourceTempAdd += 2;
								UnCommpressData = UnCommpressData + RecycleCount;
							}
					}
			

			}

				{
					if(BmpInfoHeader.biClrImportant)
						Colors = BmpInfoHeader.biClrImportant;
					else if(BmpInfoHeader.biClrUsed)
						Colors = BmpInfoHeader.biClrUsed;
					else 
						Colors = 256;

					Palettes = (U32 *)CH_AllocMem(sizeof(U32)*Colors);
					if(!Palettes)
					{
				#ifdef ENABLE_CH_PRINT	
						CH_Printf("drawbmp.c @ alloc error!\n");
				#endif
						return CH_ALLOCMEM_ERROR;
					}
					graph_read(Palettes,sizeof(U32),Colors,pFile);
					graph_seek(pFile,BmpStart,0);
					j = 0;
					width = BmpInfoHeader.biBitCount /8 * BmpInfoHeader.biWidth;
					if(width%4 != 0)
						{
                         width+=(4-width%4);
						}
					if(ReverseType)
						point.y = RectSize->iStartY;
						while(j < BmpInfoHeader.biHeight)
						{
							j ++;
							for(i=0;i<width;i++)
							{
				
							 if (IsPacket == true)
								{
					 	  Index = *UnpacketData;
						  UnpacketData++;
					}
					else
								graph_read(&Index,1,1,pFile);
								if((point.x - RectSize->iStartX) < BmpInfoHeader.biWidth)
								{
								CH_GET_RGB_COLOR((&struct_color)).uRed = (U8)((Palettes[Index]>>16)&0xff);
								CH_GET_RGB_COLOR((&struct_color)).uGreen = (U8)((Palettes[Index]>>8)&0xff);
								CH_GET_RGB_COLOR((&struct_color)).uBlue = (U8)((Palettes[Index])&0xff);
								CH_DrawColorPoint(ch_this, &point, &struct_color);
								}
								point.x ++;
							}	
						if(ReverseType)
							point.y ++;
						else
							point.y --;
							point.x = RectSize->iStartX;
						}
					graph_close(pFile);
				}
				break;
			case 16:
					if(BmpInfoHeader.biCompression == BI_RGB)
					{
						depth = BmpInfoHeader.biBitCount /8;
						width = depth * BmpInfoHeader.biWidth;
						if(width%4 != 0)
							{
							 width+=(4-width%4);
							}
						graph_seek(pFile,BmpStart,0);

							pData = (U8 *)CH_AllocMem(sizeof(char)*(width));
							if(!pData)
							{
								#ifdef ENABLE_CH_PRINT	
								CH_Printf("drawbmp.c @ alloc error!\n");
								#endif
								return CH_ALLOCMEM_ERROR;
							}
							widthlen = ch_this->SingleScrWidth * depth;
							if(ReverseType)
							{
								buffer = (U8*)(ch_this->scrBuffer.dwHandle		\
														+ RectSize->iStartY * ch_this->SingleScrWidth * ch_this->ColorByte		\
														+ RectSize->iStartX * ch_this->ColorByte);
							}
							else
							{
								buffer = (U8*)(ch_this->scrBuffer.dwHandle		\
														+ (RectSize->iStartY + (BmpInfoHeader.biHeight-1)) * ch_this->SingleScrWidth * ch_this->ColorByte		\
														+ RectSize->iStartX * ch_this->ColorByte);
								}
								for(i = 0; i < BmpInfoHeader.biHeight; i ++)
								{
									graph_read(pData, sizeof(char), width, pFile);
									tmp = pData;
									buf = buffer;
									for(j = 0; j < BmpInfoHeader.biWidth; j ++)
									{
																
										 CHZ_DrawColorPoint((tmp[1]>>2)<<3, (tmp[1]<<6)+((tmp[0]&0xE0)>>2), (tmp[0]&0x1f)<<3, buf);
										 buf +=ch_this->ColorByte;
										 tmp+= depth;
									 }
								if(ReverseType)
								buffer += ch_this->SingleScrWidth * ch_this->ColorByte;
								else
									buffer -= ch_this->SingleScrWidth * ch_this->ColorByte;
								}
							
							graph_close(pFile);
							}
					else if(BmpInfoHeader.biCompression == 3)/* 20080220 add Ã»×ÊÁÏ*/
						{
							
							graph_close(pFile);
							return CH_FAILURE;

					}
					else
					{
						graph_close(pFile);
						return CH_FAILURE;
					}
				break;
			case 24:
					if(BmpInfoHeader.biCompression == BI_RGB)
				{
					depth = BmpInfoHeader.biBitCount /8;
					width = depth * BmpInfoHeader.biWidth;
					if(width%4 != 0)
						{
                         width+=(4-width%4);
						}

					graph_seek(pFile,BmpStart,0);
						pData = (U8 *)CH_AllocMem(sizeof(char)*(width));
						if(!pData)
						{
					#ifdef ENABLE_CH_PRINT	
							CH_Printf("drawbmp.c @ alloc error!\n");
					#endif
							return CH_ALLOCMEM_ERROR;
						}
						widthlen = ch_this->SingleScrWidth * depth;
					if(ReverseType)
						{
							buffer = (U8*)(ch_this->scrBuffer.dwHandle		\
								+ RectSize->iStartY  * ch_this->SingleScrWidth * ch_this->ColorByte		\
								+ RectSize->iStartX * ch_this->ColorByte);
						}
						else
						{
							buffer = (U8*)(ch_this->scrBuffer.dwHandle		\
								+ (RectSize->iStartY + (BmpInfoHeader.biHeight-1)) * ch_this->SingleScrWidth * ch_this->ColorByte		\
								+ RectSize->iStartX * ch_this->ColorByte);
						}

								for(i = 0; i < BmpInfoHeader.biHeight; i ++)
								{
									graph_read(pData, sizeof(char), width, pFile);
									tmp = pData;
									buf = buffer;
									for(j = 0; j < BmpInfoHeader.biWidth; j ++)
									{
																
										 CHZ_DrawColorPoint((U8)tmp[2], (U8)tmp[1], (U8)tmp[0], buf);
										   
										buf +=ch_this->ColorByte;
										
										  tmp+= depth;
									}
							if(ReverseType)
								buffer += ch_this->SingleScrWidth * ch_this->ColorByte;
							else									
									buffer -= ch_this->SingleScrWidth * ch_this->ColorByte;
								}
						
					graph_close(pFile);
				}
				else
				{
					graph_close(pFile);
					return CH_FAILURE;
				}
				break;
			case 32:
				if(BmpInfoHeader.biCompression == BI_RGB)
				{
					depth = BmpInfoHeader.biBitCount /8;
					width = depth * BmpInfoHeader.biWidth;
					graph_seek(pFile,BmpStart,0);
						pData = (U8 *)CH_AllocMem(sizeof(char)*(width));
						if(!pData)
						{
					#ifdef ENABLE_CH_PRINT	
							CH_Printf("drawbmp.c @ alloc error!\n");
					#endif
							return CH_ALLOCMEM_ERROR;
						}
						widthlen = ch_this->SingleScrWidth *depth;

						if(ReverseType)
						{
							buffer = (U8*)(ch_this->scrBuffer.dwHandle		\
								+ RectSize->iStartY  * ch_this->SingleScrWidth * ch_this->ColorByte	\
								+ RectSize->iStartX * ch_this->ColorByte);
						}
						else
						{
							buffer = (U8*)(ch_this->scrBuffer.dwHandle		\
								+ (RectSize->iStartY + (BmpInfoHeader.biHeight-1)) * ch_this->SingleScrWidth * ch_this->ColorByte	\
								+ RectSize->iStartX * ch_this->ColorByte);
							}
								for(i = 0; i < BmpInfoHeader.biHeight; i ++)
								{
									graph_read(pData, sizeof(char), width, pFile);
									tmp = pData;
									buf = buffer;
									for(j = 0; j < BmpInfoHeader.biWidth; j ++)
									{
										CHZ_DrawColorPoint((U8)tmp[2], (U8)tmp[1], (U8)tmp[0], buf);
										buf += ch_this->ColorByte;
										tmp+= depth;
									}
							if(ReverseType)
								buffer += ch_this->SingleScrWidth * ch_this->ColorByte;
								else
									buffer -= ch_this->SingleScrWidth * ch_this->ColorByte;
								}
						
					graph_close(pFile);
				}
				else
				{
					graph_close(pFile);
					return CH_FAILURE;
				}
				break;
		}
	#ifdef ENABLE_CH_PRINT
	CH_Printf("draw bmp ok\n");
	#endif
	#ifdef dz_DEBUG_BMP
	STTBX_Print("2 ");
	#endif
	
	bmprect.iStartX = RectSize->iStartX;
	bmprect.iStartY = RectSize->iStartY;
	bmprect.iHeigh = (S16)BmpInfoHeader.biHeight;
	bmprect.iWidth = (S16)BmpInfoHeader.biWidth;
	CH_RepeatImageInArea(ch_this, &bmprect, RectSize, repeat);
	if(pData)
		{
		CH_FreeMem(pData);
		pData = NULL;
		}
	if(Palettes)
		{
		CH_FreeMem(Palettes);
		Palettes = NULL;
}
	/* 20080220 add */
	if(FreeDataAdd)/* 20080219 add */
{
			CH_FreeMem(FreeDataAdd);
			UnCommpressData = NULL;
			UnpacketData = NULL;
			CurSourceTempAdd = NULL;
			FreeDataAdd =NULL;
}
	return CH_SUCCESS;
}



