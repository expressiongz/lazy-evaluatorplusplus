#include <iostream>
#include "lazy_evaluator++.hpp"

void main_thread( HMODULE dll_module )
{
	std::FILE* file_stream{ nullptr };
	freopen_s( &file_stream, "CONOUT$", "w", stdout );
	AllocConsole( );

	const auto func = lazy_eval::find_hashed_function< 0xb85f6898 >( 0xd395b3e9, 0x1000193 );
	if( func.has_value( ) )
		std::printf( "found hashed function: %s\nhash: %x", func.value( ).data( ), 0xb85f6898 );
	else
		std::printf( "couldnt find hashed function\n" );

	FreeLibrary( dll_module );

}

bool __stdcall DllMain( HMODULE dll_module, const std::uintptr_t reason_for_call, void* )
{
	if( reason_for_call == DLL_PROCESS_ATTACH )
		std::thread{ main_thread, dll_module }.detach( );
	return true;
}