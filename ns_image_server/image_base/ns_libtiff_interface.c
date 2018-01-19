#include "ns_libtiff_interface.h"
#include <fcntl.h>

#include "tiffio.h"

//global variable used to store pointers to internal TIFF library functions
//This is done so we can declare our own TIFF error handlers using the TIFFlib C interface.

TIFF * ref_tif = 0;
int ns_DummyMapProc(thandle_t fd, void** pbase, toff_t* psize){return 0;}
void _tiffDummyUnmapProc(thandle_t fd, void* base, toff_t size){}

TIFF* ns_tiff_fd_open(ns_tiff_client_data * client_data, const char* name, const char* mode){
	TIFF* tif;
	#ifdef _WIN32 
	int fSuppressMap;
	int m;
	fSuppressMap = 0;
	for (m = 0; mode[m] != 0; m++)
	{
		if (mode[m] == 'u')
		{
			fSuppressMap = 1;
			break;
		}
	}
	tif = TIFFClientOpen(name, mode, client_data->tiff_fd.h, 
			     TIFFGetReadProc(ref_tif), TIFFGetWriteProc(ref_tif),
			     TIFFGetSeekProc(ref_tif), TIFFCloseProc(ref_tif), TIFFGetSizeProc(ref_tif),
			     fSuppressMap ? ns_DummyMapProc : TIFFGetMapFileProc(ref_tif),
			     fSuppressMap ? ns_DummyUnmapProc : TIFFGetUnmapFileProc(ref_tif));
	#else
	    
		tif = TIFFClientOpen(name, mode,
			client_data->tiff_fd.h,
				     TIFFGetReadProc(ref_tif), TIFFGetWriteProc(ref_tif),
				     TIFFGetSeekProc(ref_tif), TIFFGetCloseProc(ref_tif), TIFFGetSizeProc(ref_tif),
				     TIFFGetMapFileProc(ref_tif), TIFFGetUnmapFileProc(ref_tif));
	

	#endif	
	if (tif)
	  TIFFSetFileno(tif, client_data->tiff_fd.fd);
       
	return (tif);
} 

int ns_TIFFgetMode(const char* mode, const char* module)
{
	int m = -1;

	switch (mode[0]) {
	case 'r':
		m = O_RDONLY;
		if (mode[1] == '+')
			m = O_RDWR;
		break;
	case 'w':
	case 'a':
		m = O_RDWR|O_CREAT;
		if (mode[0] == 'w')
			m |= O_TRUNC;
		break;
	default:
		TIFFErrorExt(0, module, "\"%s\": Bad mode", mode);
		break;
	}
	return (m);
}


TIFF* ns_tiff_open(const char* name, ns_tiff_client_data * client_data,const char* mode){
  //We attempt to load a file here, not to open anything
  //but to get pointers to the internal TIFF library functions
  //so we can use them for our own handlers.
  if (ref_tif==0)
    ref_tif = TIFFFdOpen(0,"tst.tif","w");
 
  //printf("NS OPENING %s\n",name); 
#ifdef _WIN32 
	static const char module[] = "TIFFOpen";

	int m;
	DWORD dwMode;
	TIFF* tif;

	m = ns_TIFFgetMode(mode, module);

	switch (m) {
	case O_RDONLY:			dwMode = OPEN_EXISTING; break;
	case O_RDWR:			dwMode = OPEN_ALWAYS;   break;
	case O_RDWR | O_CREAT:		dwMode = OPEN_ALWAYS;   break;
	case O_RDWR | O_TRUNC:		dwMode = CREATE_ALWAYS; break;
	case O_RDWR | O_CREAT | O_TRUNC:	dwMode = CREATE_ALWAYS; break;
	default:			return ((TIFF*)0);
	}
	client_data->tiff_fd.fd = (thandle_t)CreateFileA(name,
		(m == O_RDONLY)?GENERIC_READ:(GENERIC_READ | GENERIC_WRITE),
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, dwMode,
		(m == O_RDONLY)?FILE_ATTRIBUTE_READONLY:FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (client_data->tiff_fd.fd == INVALID_HANDLE_VALUE) {
		TIFFErrorExt(0, module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}
	tif = ns_tiff_fd_open(client_data, name, mode);
	if(!tif)
		CloseHandle(client_data->tiff_fd.fd);
	return tif;
#else
	static const char module[] = "TIFFOpen";
	int m;
        TIFF* tif;

	m = ns_TIFFgetMode(mode, module);
	if (m == -1)
		return ((TIFF*)0);
	#ifdef O_BINARY
        m |= O_BINARY;
	#endif        
	#ifdef _AM29K
	client_data->tiff_fd.fd = open(name, m);
	#else
	client_data->tiff_fd.fd = open(name, m, 0666);
	#endif
	if (client_data->tiff_fd.fd < 0) {
	  //	if (errno > 0 && strerror(errno) != NULL) {
	  //		TIFFErrorExt(0, module, "%s: %s", name, strerror(errno));
	  //	}
	  //	else {
			TIFFErrorExt(0, module, "%s: Cannot open", name);
	  //	}
		return ((TIFF *)0);
	}


	tif = ns_tiff_fd_open(client_data, name, mode);
	if(!tif)
		close(client_data->tiff_fd.fd);
	return tif;
#endif
 
}
