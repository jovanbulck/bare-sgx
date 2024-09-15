Sample output:

```
[load.c] parsing enclave binary 'enclave/encl.elf'..
[load.c] measuring enclave binary..
[load.c] loading enclave binary..
[load.c] mmapping loaded enclave..
encl {
    fd: 3
    bin: 0x7f3acc9b9000
    bin_size: 17168
    src: 0x7f3acc9ba000
    src_size: 20480
    encl_size: 32768
    encl_base: 7f3acc9a0000
    nr_segments: 4
    segment_tbl: [
        encl_segment {
            src: 0x7f3acc9ba000
            offset: 0
            size: 4096
            prot: rw-
            flags: ---; page type=TCS
            measure: true
        }
        encl_segment {
            src: 0x7f3acc9bb000
            offset: 4096
            size: 4096
            prot: r-x
            flags: r-x; page type=REG
            measure: true
        }
        encl_segment {
            src: 0x7f3acc9bc000
            offset: 8192
            size: 8192
            prot: rw-
            flags: rw-; page type=REG
            measure: true
        }
        encl_segment {
            src: 0x7f3acc9b8000
            offset: 16384
            size: 4096
            prot: rw-
            flags: rw-; page type=REG
            measure: false
        }
    ]
    sgx_secs {
        size: 32768
        base: 7f3acc9a0000
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
[main.c] loaded enclave at 0x7f3acc9a0000
[main.c] reading enclave memory..
	L mem at 0x7f3acc9a0000 is ffffffffffffffff
[main.c] calling enclave TCS..
	L enclave returned deadbeefcafebabe
```
