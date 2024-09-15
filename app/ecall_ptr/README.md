Sample output:

```
[load.c] parsing enclave binary 'enclave/encl.elf'..
[load.c] measuring enclave binary..
[load.c] loading enclave binary..
[load.c] mmapping loaded enclave..
encl {
    fd: 3
    bin: 0x7f05092b5000
    bin_size: 25552
    src: 0x7f05092b6000
    src_size: 28672
    encl_size: 32768
    encl_base: 7f05092a0000
    nr_segments: 4
    segment_tbl: [
        encl_segment {
            src: 0x7f05092b6000
            offset: 0
            size: 4096
            prot: rw-
            flags: ---; page type=TCS
            measure: true
        }
        encl_segment {
            src: 0x7f05092b7000
            offset: 4096
            size: 8192
            prot: r-x
            flags: r-x; page type=REG
            measure: true
        }
        encl_segment {
            src: 0x7f05092b9000
            offset: 12288
            size: 12288
            prot: rw-
            flags: rw-; page type=REG
            measure: true
        }
        encl_segment {
            src: 0x7f05092b4000
            offset: 24576
            size: 4096
            prot: rw-
            flags: rw-; page type=REG
            measure: false
        }
    ]
    sgx_secs {
        size: 32768
        base: 7f05092a0000
        ssa_frame_size: 1
        miscselect: 0
        attributes: 4
        xfrm: 3
        mrenclave: [0, 0, 0, 0, 0, 0, 0, 0]
        mrsigner: [0, 0, 0, 0, 0, 0, 0, 0]
        config_id: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        isv_prod_id: 0
        isv_svn: 0
        config_svn: 0
    }
}
[main.c] loaded enclave at 0x7f05092a0000
[main.c] reading enclave memory..
	L mem at 0x7f05092a0000 is ffffffffffffffff
[main.c] calling enclave TCS..
	L enclave returned 1300 + 37 = 1337
	L enclave returned 1300 - 37 = 1263
```
