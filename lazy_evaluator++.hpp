#include <Windows.h>
#include <winternl.h>
#include <optional>
#include <vector>
namespace util
{

	struct ldr_data_list_entry_t64
    {
        LIST_ENTRY links;
        std::uint8_t pad_00[ 16 ];

        std::uintptr_t module_base;
        std::uintptr_t module_entry_point;
        std::size_t module_image_sz;

        UNICODE_STRING module_path;
        UNICODE_STRING module_name;
    };
	 
	struct ldr_data_list_entry_t32
    {
        LIST_ENTRY links;
        std::uint8_t pad_00[ 8 ];

        std::uintptr_t module_base;
        std::uintptr_t module_entry_point;
        std::size_t module_image_sz;

        UNICODE_STRING module_path;
        UNICODE_STRING module_name;
    };
#ifdef _M_X64
	using ldr_data_list_entry_t = ldr_data_list_entry_t64;
#else
	using ldr_data_list_entry_t = ldr_data_list_entry_t32;
#endif
	[[ nodiscard ]] std::vector< std::string_view > get_import_names( )
	{
#ifdef _M_X64
		const auto peb = reinterpret_cast< PEB* >( __readgsqword( 0x60 ) );
#else
		const auto peb = reinterpret_cast< PEB* >( __readfsdword( 0x30 ) );
#endif
		const auto ldr_data_list = peb->Ldr->InMemoryOrderModuleList;
		const auto first_module = ldr_data_list.Flink;

		const auto last_module = ldr_data_list.Blink;
		auto current_module = first_module;
		
		std::vector< std::string_view > import_names{ };

		for( ; ; current_module = current_module->Flink )
		{

			const auto current_entry = reinterpret_cast< ldr_data_list_entry_t* >( current_module );
			const auto module = current_entry->module_base;

			const auto module_dos_header = reinterpret_cast< IMAGE_DOS_HEADER* >( module );
			const auto module_nt_headers = reinterpret_cast< IMAGE_NT_HEADERS* >( module + module_dos_header->e_lfanew );

			const auto module_optional_header = module_nt_headers->OptionalHeader;
			const auto export_dir = reinterpret_cast< IMAGE_EXPORT_DIRECTORY* >( module + module_optional_header.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress );

			const auto function_names = reinterpret_cast< std::uint32_t* >( module + export_dir->AddressOfNames );
			const auto num_functions = export_dir->NumberOfFunctions;

			for( auto i = 0u; i < num_functions; ++i )
				import_names.push_back( std::string_view( reinterpret_cast< char* >( module + function_names[ i ] ) ) );

			if( current_entry->module_base == reinterpret_cast< ldr_data_list_entry_t* >( last_module )->module_base )
				break;

		}
		return import_names;
	}
}

namespace lazy_eval
{

	[[ nodiscard ]] __forceinline std::uintptr_t hash_import_name( std::string_view import_name, std::uintptr_t seed, std::uintptr_t prime )
	{
		auto seed_buf{ seed };
		for( auto it = import_name.begin( ); it < import_name.end( ); ++it )
			seed = prime * ( seed ^ *it );
		return seed;
	}

	template< auto hash >
	[[ nodiscard ]] std::optional< std::string_view > find_hashed_function( std::uintptr_t seed, std::uintptr_t prime )
	{
		const auto function_names = util::get_import_names( );
		
		for( const auto& function_name : function_names )
		{
			if( static_cast< decltype( hash ) >( hash_import_name( function_name.data( ), seed, prime ) ) == hash )
				return function_name;
		}
		return std::nullopt;
	}
}
