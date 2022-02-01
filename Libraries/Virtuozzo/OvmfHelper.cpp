#include "OvmfHelper.h"


// return NVRAM template for vz8 and vz7
QString OVMF::getTemplate(Chipset_type machine_type)
{
	return machine_type == Chipset_type::Q35 ? "/usr/share/OVMF/OVMF_VARS.secboot.fd":
											   "/usr/share/OVMF/OVMF_VARS.piix.fd";
}

//return firmware for vz8 and vz7
//SMM-enabled for vz8 and SMM-off for vz7, respectively
QString OVMF::getFirmware(Chipset_type machine_type)
{
	return machine_type == Chipset_type::Q35 ? "/usr/share/OVMF/OVMF_CODE.secboot.fd":
											   "/usr/share/OVMF/OVMF_CODE.piix.fd";
}
