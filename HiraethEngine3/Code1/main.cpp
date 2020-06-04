
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
#define DBG_NEW new
#endif

struct Pod {
	int x;
};



int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	/*Pod* pPod = DBG_NEW Pod;
	pPod = DBG_NEW Pod; // Oops, leaked the original pPod!
	delete pPod;
	*/

	int x = 0;
	x += 10;

	_CrtDumpMemoryLeaks(); 
	return 0;
};