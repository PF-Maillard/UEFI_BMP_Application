#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>

#include <Library/BaseLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/SimpleFileSystem.h>
#include <IndustryStandard/Bmp.h>
#include <Guid/FileInfo.h>

typedef struct Bullet_N
{
	UINTN x;
	UINTN y;
	struct Bullet_N * Next;
} Bullet;

EFI_STATUS AddBullet(IN OUT Bullet ** A, IN UINTN x, IN UINTN y)
{
	EFI_STATUS Status;
	Bullet * New;

	//
	// If the Bullet List is NULL
	//
	if (A == NULL)
		return EFI_UNSUPPORTED;

	//
	// If the node is empty, add a Bullet at the position
	//
	if (*A == NULL)
	{
		Status = gBS->AllocatePool(EfiBootServicesData, sizeof(Bullet), (VOID **)&New);
		if (EFI_ERROR(Status))
		{
			Print(L"ERROR (AddBullet): Impossible d'allouer de la memoire\n");
			return Status;
		}

		New->x = x;
		New->y = y;
		New->Next = NULL;

		*A = New;
		return EFI_SUCCESS;
	}
	
	//
	// Go to the next node
	//
	return AddBullet(&((*A)->Next), x, y);
}

EFI_STATUS ShowBullet(IN OUT Bullet * BulletListe, IN EFI_GRAPHICS_OUTPUT_PROTOCOL * GraphicsProtocol)
{
	EFI_STATUS Status;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL White = { 0xFF, 0xFF, 0xFF, 0 };

	//
	// If the Bullet List is NULL
	//
	if (BulletListe == NULL)
		return EFI_SUCCESS;

	//
	// If it exist a Bullet, display it
	//
	Status = GraphicsProtocol->Blt(GraphicsProtocol, &White, EfiBltVideoFill, 0, 0, BulletListe->x, BulletListe->y, 10, 10, 0);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (ShowBullet): Impossible d'utiliser le protocole graphique\n");
		return EFI_UNSUPPORTED;
	}

	
	//
	// Go to the next node
	//
	return ShowBullet(BulletListe->Next, GraphicsProtocol);
}

EFI_STATUS MoveBullet(IN OUT Bullet ** A)
{
	Bullet * Next;

	//
	// If the Bullet List is NULL, return an error
	//
	if (A == NULL)
		return EFI_UNSUPPORTED;
	
	
	//
	// If it is the end of the list, exit 
	//
	if (*A == NULL)
		return EFI_SUCCESS;

	 Next = ((*A)->Next);
	
	//
	// Advance each bullet
	//
	if (((*A)->y - 5) >= 0)
		(*A)->y -= 5;
	else
		(*A)->y = 0;

	//
	// Go to the next node
	//
	return MoveBullet(&Next);
}

EFI_STATUS DestroyEndBullet(IN OUT Bullet ** A, IN Bullet * Last)
{
	Bullet * Next;

	//
	// If the Bullet List is NULL, return an error
	//
	if (A == NULL)
		return EFI_UNSUPPORTED;
	
	
	//
	// If it is the end of the list, exit 
	//
	if (*A == NULL)
		return EFI_SUCCESS;

	Next = (*A)->Next;

	
	//
	// If the bullet is out of the screen, destroy it
	//
	if ((*A)->y == 0)
	{

		if (Last == NULL)
		{
			if (Next == NULL)
			{
				//
				// If it is the only element of the list, the list become NULL
				//
				FreePool(*A);
				*A = NULL;
				return DestroyEndBullet(&Next, Last);
			}
			else
			{ 
				//
				// if it is the first element, take the next element
				//
				FreePool(*A);
				*A = Next;
				return DestroyEndBullet(&Next, NULL);
			}
		}
		else
		{
			//
			// if the element at the center of the list, free it and reattach the list
			//
			Last->Next = Next;
			FreePool(*A);
			return DestroyEndBullet(&Next, Last);
		}
	}
	else
		//
		// Loop
		//
		return DestroyEndBullet(&((*A)->Next), *A);
}

EFI_STATUS ConvertBmpToBlt(IN VOID *BmpImage, IN UINTN BmpImageSize, IN OUT VOID **GopBlt, IN OUT UINTN *GopBltSize, OUT UINTN *PixelHeight, OUT UINTN *PixelWidth)
{

	UINT8                         *ImageData;
	UINT8                         *ImageBegin;
	BMP_IMAGE_HEADER              *BmpHeader;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
	UINT64                        BltBufferSize;
	UINTN                         Height;
	UINTN                         Width;
	UINTN                         ImageIndex;

	BmpHeader = (BMP_IMAGE_HEADER *)BmpImage;
	ImageBegin = ((UINT8 *)BmpImage) + BmpHeader->ImageOffset;

	if (BmpHeader->BitPerPixel != 24 && BmpHeader->BitPerPixel != 32)
		return EFI_UNSUPPORTED;

	BltBufferSize = BmpHeader->PixelWidth * BmpHeader->PixelHeight * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
	*GopBltSize = (UINTN)BltBufferSize;
	*GopBlt = AllocatePool(*GopBltSize);   
	if (*GopBlt == NULL)
	{
		return EFI_OUT_OF_RESOURCES;
	}

	*PixelWidth = BmpHeader->PixelWidth;
	*PixelHeight = BmpHeader->PixelHeight;

	ImageData = ImageBegin;
	BltBuffer = *GopBlt;
	for (Height = 0; Height < BmpHeader->PixelHeight; Height++)
	{
		Blt = &BltBuffer[(BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
		for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Blt++)
		{
			switch (BmpHeader->BitPerPixel)
			{
			case 24:
				Blt->Blue = *ImageData++;
				Blt->Green = *ImageData++;
				Blt->Red = *ImageData++;
				break;
			case 32:
				ImageData++;
				Blt->Blue = *ImageData++;
				Blt->Green = *ImageData++;
				Blt->Red = *ImageData++;
				break;
			default:
				break;
			}
		}

		ImageIndex = (UINTN)(ImageData - ImageBegin);
		if ((ImageIndex % 4) != 0)
			ImageData = ImageData + (4 - (ImageIndex % 4));
	}
	
	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable)
{
	EFI_STATUS Status;
	EFI_GRAPHICS_OUTPUT_PROTOCOL * GraphicsProtocol = NULL;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL Black = { 0x00, 0x00, 0x00, 0 };
	UINTN EventIndex;
	EFI_EVENT          TimerEvent;
	EFI_EVENT          WaitList[2];
	EFI_INPUT_KEY Key;
	UINTN						 x, y;
	VOID						 *GopBlt = NULL;
	UINTN					 	 GopBltSize;
	UINTN						 BmpHeight;
	UINTN						 BmpWidth;
	EFI_FILE_PROTOCOL *Root = 0;
	EFI_FILE_PROTOCOL *ReadMe = 0;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;
	EFI_FILE_INFO *fileinfo;
	VOID * Buffer;
	UINTN BufferSize;
	Bullet * BulletListe = NULL;

	UINTN infosize = SIZE_OF_EFI_FILE_INFO;
	EFI_GUID info_type = EFI_FILE_INFO_ID;

	//
	// File recuperation
	//
	Status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&SimpleFileSystem);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible de récupérer le protocole de lecture de fichier\n");
		return Status;
	}

	Status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &Root);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible d'associer la racine au protocole \n");
		return Status;
	}

	Status = Root->Open(Root, &ReadMe, (CHAR16*)L"Image.bmp", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible d'ouvrir le fichier contenant l'image \n");
		return Status;
	}

	//
	// Information file recuperation
	//
	Status = gBS->AllocatePool(AllocateAnyPages, infosize, (VOID **)&fileinfo);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible d'allouer la mémoire pour les informations du fichier\n");
		return Status;
	}

	Status = ReadMe->GetInfo(ReadMe, &info_type, &infosize, NULL);
	Status = ReadMe->GetInfo(ReadMe, &info_type, &infosize, fileinfo);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible de récupérer les informations du fichier\n");
		return Status;
	}

	BufferSize = fileinfo->FileSize;
	gBS->AllocatePool(AllocateAnyPages, BufferSize, (VOID **)&Buffer);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible d'allouer de la memoire\n");
		return Status;
	}

	//
	// Buffer recuperation
	//
	Status = ReadMe->Read(ReadMe, &BufferSize, Buffer);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible de fermer le fichier \n");
		return Status;
	}

	//
	// End of file reading
	// 
	//
	Status = ReadMe->Close(ReadMe);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible de fermer le fichier \n");
		return Status;
	}

	//
	// BMP conversion
	//
	Status = ConvertBmpToBlt(Buffer, BufferSize, &GopBlt, &GopBltSize, &BmpHeight, &BmpWidth);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible de convertir en Image\n");
		return Status;
	}


	//
	// grpahic protocol recuperation
	//
	Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&GraphicsProtocol);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible de recupérer le protocole graphique\n");
		return Status;
	}

	x = 0;
	y = 0;
	do
	{
		//
		// Image display
		//
		Status = GraphicsProtocol->Blt(GraphicsProtocol, &Black, EfiBltVideoFill, 0, 0, 0, 0, 800, 600, 0);
		if (EFI_ERROR(Status))
		{
			Print(L"ERROR (main): Impossible d'utiliser le protocole graphique\n");
			return Status;
		}

		Status = GraphicsProtocol->Blt(GraphicsProtocol, GopBlt, EfiBltBufferToVideo, 0, 0, x, y, BmpWidth, BmpHeight, 0);
		if (EFI_ERROR(Status))
		{
			Print(L"ERROR (main): Impossible d'utiliser le protocole graphique\n");
			return Status;
		}

		ShowBullet(BulletListe, GraphicsProtocol);

		
		//
		// Creation of 2 events: keyboard press or timer. With this, Bullets can advance without the touch of a button 
		// (to improve, synchronize the 2 so that the Bullets go exactly the same speed between a key press and without) 
		//
		Status = gBS->CreateEvent(EVT_TIMER, 0, NULL, NULL, &TimerEvent);
		if (EFI_ERROR(Status))
		{
			Print(L"%r", Status);
			Print(L"ERROR (main): Impossible de creer un event de timer\n");
			return Status;
		}

		Status = gBS->SetTimer(TimerEvent, TimerRelative, 1000000);
		if (EFI_ERROR(Status))
		{
			Print(L"%r", Status);
			Print(L"ERROR (main): Impossible de mettre un timer\n");
			return Status;
		}

		WaitList[0] = gST->ConIn->WaitForKey;
		WaitList[1] = TimerEvent;

		Status = gBS->WaitForEvent(2, WaitList, &EventIndex);
		if (EFI_ERROR(Status))
		{
			Print(L"ERROR (main): Impossible d'attendre un event\n");
			return Status;
		}
		
		//
		// If it's a key press, we move the ship
		//
		if (EventIndex == 0)
		{ 
			Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
			if (EFI_ERROR(Status))
			{
				Print(L"ERROR (main): Impossible de recuperer une touche\n");
				return Status;
			}

			switch (Key.UnicodeChar)
			{
			case 'w':
				if (y > 10)
					y -= 10;
				break;
			case 's':
				if (y < 600 - 50)
					y += 10;
				break;
			case 'a':
				if (x > 10)
					x -= 10;
				break;
			case 'd':
				if (x < 800 - 50)
					x += 10;
				break;
			case 'f':
				AddBullet(&BulletListe, x + BmpWidth/2, y);
				break;
			}
		}
	MoveBullet(&BulletListe);
	DestroyEndBullet(&BulletListe, NULL);
	} while (Key.UnicodeChar != 'y');
				
	//
	// we advance the Bullets an	d destroy those coming out of the screen
	//
	Status = MoveBullet(&BulletListe);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible de bouger un Bullet\n");
		return Status;
	}
	Status = DestroyEndBullet(&BulletListe, NULL);
	if (EFI_ERROR(Status))
	{
		Print(L"ERROR (main): Impossible de detruire les bullets\n");
		return Status;
	}

	//
	// free element
	//
	gBS->FreePool(fileinfo);
	gBS->FreePool(Buffer);

	return EFI_SUCCESS;
}