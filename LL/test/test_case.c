void fun()
{
	switch (ent->type) {
		case NUBUS_RESID_MAC_ADDRESS:
			{
				nubus_get_rsrc_mem(addr, ent, 6);
				printk( "    MAC address: ");
				for (i = 0; i < 6; i++)
					printk("%02x%s", addr[i] & 0xff,
							i == 5 ? "" : ":");
				printk("\n");
				break;
			}
		default:
			printk( "    unknown resource %02X, data 0x%06x\n",
					ent->type, ent->data);
	}
	switch(i) {
		case 1: {
				a = b + c;
				break;
			}
		case ADD:
				break;
		case SUB:
				a = b - c;
		case MUL: {
				a = b * c;
				break;
			}
		case 'c':
				return ret;
		case INNER:
				switch(j) {
					case i:
						break;
				}
		default: {
				a = b / c;
				break;
			}
	}
}
