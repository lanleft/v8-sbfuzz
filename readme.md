
# Table of content

- [Setup](#setup)


# Setup 

1. Install v8, depot_tools+AFLplusplus

- Remember changing UID:GID in Dockerfile before building


```bash
# install depot_tools
# Conditional git clone if the directory is empty
# RUN mkdir -p /home/vult/depot_tools && \
#     [ "$(ls -A /home/vult/depot_tools)" ] || git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git /home/vult/depot_tools
# ENV PATH="/home/vult/depot_tools:${PATH}"

# # install AFLplusplus
# # Conditional git clone if the directory is empty
# # AFLplusplus version 4.10a
# RUN mkdir -p /home/vult/AFLplusplus && \
#     [ "$(ls -A /home/vult/AFLplusplus)" ] || (\
#     git clone https://github.com/AFLplusplus/AFLplusplus.git /home/vult/AFLplusplus && \
#     cd /home/vult/AFLplusplus && \
#     git checkout ca0c9f6d1797bac121996c3b2ac50423f6e67b8f)
```

2. run dockerfile: `sudo docker compose build && sudo docker compose run v8 zsh`


3. Build AFL++ and v8 release+debug

```bash
cd AFLplusplus
make distrib
sudo make install

```


# Unicorn fuzzer

Blogs:
    - https://eternalsakura13.com/2020/03/18/unicorn_learn/
    - https://hackernoon.com/afl-unicorn-part-2-fuzzing-the-unfuzzable-bea8de3540a5

## Dumpper

```bash
source /home/vult/v8_sb_fuzz/AFLplusplus/unicorn_mode/helper_scripts/unicorn_dumper_pwndbg.py
```

Example 1:

![alt text](image.png)


**Harness**

- C: https://github.com/AFLplusplus/AFLplusplus/blob/stable/unicorn_mode/samples/c/harness.c
- Python: https://github.com/AFLplusplus/AFLplusplus/blob/stable/unicorn_mode/samples/python_simple/simple_test_harness.py
- Unicorn-fuzz: https://github.com/unicorn-engine/unicorn/blob/master/tests/fuzz/fuzz_emu_x86_64.c

