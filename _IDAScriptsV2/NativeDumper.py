import idc
import idaapi
import idautils
import ida_hexrays

class TechlandHardenedGen:
    def __init__(self):
        self.CRTTI_Ctor = 0xc55590
        self.AddField = 0xc5ca80
        
    def get_val(self, arg):
        if arg is None: return None
        if arg.op == ida_hexrays.cot_obj: return arg.obj_ea
        if arg.op == ida_hexrays.cot_num: return arg.numval()
        if arg.op == ida_hexrays.cot_cast: return self.get_val(arg.x)
        if arg.op == ida_hexrays.cot_ref: return self.get_val(arg.x)
        return None

    def run(self, target_name):
        print(f"[V2] Hardened Analysis: {target_name}")
        
        # 1. Find string
        name_ea = idc.BADADDR
        for s in idautils.Strings():
            if str(s) == target_name:
                name_ea = s.ea
                break
        
        if name_ea == idc.BADADDR:
            print("Error: String not found.")
            return

        # 2. Find Call site using simple instruction scanning
        reg_func_ea = None
        class_size = 0
        
        for xref in idautils.XrefsTo(name_ea):
            # Look at the instruction at the xref
            if idc.print_insn_mnem(xref.frm) != "lea": continue
            
            # Scan forward for the next call
            curr = xref.frm
            for _ in range(20):
                curr = idc.next_head(curr)
                if idc.print_insn_mnem(curr) == "call" and idc.get_operand_value(curr, 0) == self.CRTTI_Ctor:
                    # Found it! Now decompile JUST THIS ONE function
                    try:
                        cfunc = ida_hexrays.decompile(curr)
                        for item in cfunc.treeitems:
                            if item.op == ida_hexrays.cot_call:
                                e = item.to_specific_type()
                                if e.x.op == ida_hexrays.cot_obj and e.x.obj_ea == self.CRTTI_Ctor:
                                    if self.get_val(e.a[1]) == name_ea:
                                        reg_func_ea = self.get_val(e.a[3])
                                        class_size = self.get_val(e.a[10])
                                        break
                    except: pass
                if reg_func_ea: break
            if reg_func_ea: break

        if not reg_func_ea:
            print("Error: Could not locate member registration function.")
            return

        print(f"Success: Found Member Registry at {hex(reg_func_ea)} | Size: {class_size}")
        
        # 3. Extract members
        print("\n" + "="*60)
        print(f"class {target_name} {{")
        
        try:
            cfunc_reg = ida_hexrays.decompile(reg_func_ea)
            members = []
            for item in cfunc_reg.treeitems:
                if item.op == ida_hexrays.cot_asg:
                    e = item.to_specific_type()
                    if e.x.op == ida_hexrays.cot_mptr and e.x.m == 80:
                        val = self.get_val(e.y)
                        if isinstance(val, int):
                            members.append(val)
            
            for off in sorted(members):
                print(f"    uintptr_t m_field_{hex(off)}; // Offset: {hex(off)}")
        except:
            print("    // Failed to extract fields.")
            
        print("};")
        print("="*60)

if __name__ == "__main__":
    if ida_hexrays.init_hexrays_plugin():
        gen = TechlandHardenedGen()
        gen.run("CoCameraSensor")
    else:
        print("Error: Hex-Rays required.")
