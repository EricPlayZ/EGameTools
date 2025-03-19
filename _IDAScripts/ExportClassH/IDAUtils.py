from functools import cache
import idc

from ExportClassH import Utils

# def GetDemangledExportedSigs() -> list[str]:
#     """
#     Generate a list of demangled function signatures from IDA's database.
#     Uses a set to avoid duplicate entries.
#     """
#     sigs_set = set()
#     entry_qty = idc.get_entry_qty()
#     for i in range(entry_qty):
#         ea: int = idc.get_entry(i)
#         exportedSig: str = idc.get_func_name(ea) or idc.get_name(ea)
#         if not exportedSig:
#             continue
#         demangledExportedSig: str = Utils.DemangleSig(exportedSig)
#         if demangledExportedSig and "~" not in demangledExportedSig and not demangledExportedSig.endswith("::$TSS0") and "::`vftable'" not in demangledExportedSig:
#             sigs_set.add(demangledExportedSig)
#     return list(sigs_set)

def GetDemangledExportedSigs() -> list[str]:
    """
    Generate a list of demangled function signatures from IDA's database.
    Uses a set to avoid duplicate entries based on (ordinal, signature).
    """
    sigs_set = set()
    entry_qty = idc.get_entry_qty()
    for i in range(entry_qty):
        ea: int = idc.get_entry(i)
        #exportedSig: str = idc.get_func_name(ea) or idc.get_name(ea)
        exportedSig = idc.get_entry_name(i)
        if not exportedSig:
            continue
        demangledExportedSig: str = Utils.DemangleSig(exportedSig)
        if demangledExportedSig and "~" not in demangledExportedSig and not demangledExportedSig.endswith("::$TSS0") and "::`vftable'" not in demangledExportedSig:
            sigs_set.add((i + 1, demangledExportedSig))
    
    return [sig for _, sig in sorted(sigs_set)]