#include <pch.h>

namespace Utils {
	namespace RTTI {
		static std::vector<DWORD64> getXrefsTo(DWORD64 moduleBaseAddr, DWORD64 address, DWORD64 start, size_t size) {
			std::vector<DWORD64> xrefs = {};

			const DWORD64 finalOffset = address - moduleBaseAddr;
			const std::string idaPattern = Memory::BytesToIDAPattern(reinterpret_cast<BYTE*>(const_cast<DWORD64*>(&finalOffset)), 4);
			std::string patt = Memory::ConvertSigToScannerSig(idaPattern.c_str());

			// Get the end of the section (in our case the end of the .rdata section)
			const DWORD64 end = start + size;

			while (start && start < end) {
				const auto scanner = memscan::mapped_region_t(start, end);
				auto patternFind = scanner.find_pattern<ms_uptr_t>(patt);

				// If the xref is 0 it means that there either were no xrefs, or there are no remaining xrefs.
				// So we should break out of our loop, otherwise it will keep on trying to look for xrefs.
				if (!patternFind.has_value())
					break;

				// We've found an xref, save it in the vector, and add 8 to start, so it wil now search for xrefs
				// from the previously found xref untill we're at the end of the section, or there aren't any xrefs left.
				xrefs.push_back(patternFind.value());
				start = patternFind.value() + 8;
			}

			return xrefs;
		}
		static bool getSectionInfo(DWORD64 baseAddress, const char* sectionName, DWORD64& sectionStart, DWORD64& sectionSize) {
			const PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(baseAddress);
			if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
				return false;

			const PIMAGE_NT_HEADERS32 ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS32>(baseAddress + dosHeader->e_lfanew);
			if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
				return false;

			PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
			USHORT numOfSections = ntHeaders->FileHeader.NumberOfSections;
			while (numOfSections > 0) {
				// If we're at the right section
				if (!strcmp(sectionName, (const char*)sectionHeader->Name)) {
					sectionStart = baseAddress + sectionHeader->VirtualAddress;
					sectionSize = sectionHeader->SizeOfRawData;

					return true;
				}

				sectionHeader++;
				numOfSections--;
			}

			return false;
		}

		DWORD64 GetVTablePtr(const char* moduleName, const char* tableName) {
			if (!moduleName || !tableName)
				return 0;

			DWORD64 baseAddress = reinterpret_cast<DWORD64>(GetModuleHandleA(moduleName));
			if (!baseAddress)
				return 0;

			// Type descriptor names look like this: .?AVFloatPlayerVariable@@ (so: ".?AV" + table_name + "@@")
			const std::string typeDescriptorName = ".?AV" + std::string(tableName) + "@@";

			// Convert the string to an IDA pattern so that we can pattern scan it
			std::string idaPattern = Memory::BytesToIDAPattern(reinterpret_cast<BYTE*>(const_cast<char*>(typeDescriptorName.data())), typeDescriptorName.size());

			DWORD64 rttiTypeDescriptor = reinterpret_cast<DWORD64>(Utils::SigScan::PatternScanner::FindPattern(moduleName, { idaPattern.c_str(), Utils::SigScan::PatternType::Address }));
			if (!rttiTypeDescriptor)
				return 0;

			// We're doing - 0x10 here, because the location of the rtti_type_descriptor is 0x10 bytes before the string (open up gamedll_ph_x64_rwdi.dll in IDA and take a look) 
			rttiTypeDescriptor -= 0x10;

			// We only need to get xrefs that are inside the .rdata section (there sometimes are xrefs in .text, so we have to filter them out)
			DWORD64 rdataStart = 0;
			DWORD64 rdataSize = 0;
			if (!getSectionInfo(baseAddress, ".rdata", rdataStart, rdataSize))
				return 0;

			// Get all xrefs to the type_descriptor
			const std::vector<DWORD64> xrefs = getXrefsTo(baseAddress, rttiTypeDescriptor, rdataStart, rdataSize);
			for (const DWORD64 xref : xrefs) {
				// xref - 0x8 = offset of this vtable in complete class (from top)
				// So if it's 0 it means it's the class we need, and not some class it inherits from (again, opening up gamedll_ph_x64_rwdi.dll in IDA will help you understand)
				const int offsetFromClass = *reinterpret_cast<int*>(xref - 0x8);
				if (offsetFromClass != 0)
					continue;

				// We've found the correct xref, the object locator is 0xC bytes before the xref. (Again, gamedll_ph_x64_rwdi.dll yada yada yada)
				const DWORD64 objectLocator = xref - 0xC;

				// Now we need to get an xref to the object locator, as that's where the vtable is located
				{
					// Convert the object locator address to an IDA pattern
					idaPattern = Memory::BytesToIDAPattern(reinterpret_cast<BYTE*>(const_cast<DWORD64*>(&objectLocator)), 8);

					const DWORD64 vtableAddr = reinterpret_cast<DWORD64>(Utils::SigScan::PatternScanner::FindPattern(reinterpret_cast<LPVOID>(rdataStart), rdataSize, { idaPattern.c_str(), Utils::SigScan::PatternType::Address })) + 0x8;

					// Here I'm checking for <= 8 as we're adding 0x8 to it. So if the pattern scan returns 0 we still head the fuck out
					if (vtableAddr <= 8)
						continue;

					return vtableAddr;

					// We've now found the vtable address, however, we probably want a pointer to the vtable (which is in .text).
					// To do this, we're going to find a reference to the vtable address, and use that as pointer.

					// If you don't need a pointer to the vtable in your implementation however, just return vtable_address
					/*DWORD64 textStart = 0;
					DWORD64 textSize = 0;
					if (!getSectionInfo(baseAddress, ".text", textStart, textSize))
						return nullptr;

					// Convert the vtable address to an IDA pattern
					idaPattern = bytesToIDAPattern(reinterpret_cast<BYTE*>(const_cast<DWORD64*>(&vtableAddr)), 8);
					return reinterpret_cast<DWORD64*>(Utils::SigScan::PatternScanner::FindPattern(reinterpret_cast<LPVOID>(textStart), textSize, { idaPattern.c_str(), Utils::SigScan::PatternType::Address })) + 0x8;*/
				}
			}

			// We for some odd reason didn't find any valid xrefs
			return 0;
		}
	}
}