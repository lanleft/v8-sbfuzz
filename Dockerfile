# Use Ubuntu 20.04 as the base image
FROM ubuntu:20.04


# Install essential packages, add more if needed
RUN apt-get update && apt-get install -y software-properties-common
RUN apt-get install python3 python3-pip wget git curl zsh -y
RUN apt-get install -y build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools

# Add LLVM's repository
RUN wget -O - https://apt.llvm.org/llvm.sh | bash -s 19

# Install LLVM 19 packages
RUN apt-get update && apt-get install -y \
llvm-19 \
clang-19 \
lldb-19 \
lld-19

# Optionally set clang-19 as the default clang
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100 \
&& update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100

# Set environment variables (optional)
ENV CC=/usr/bin/clang-19
ENV CXX=/usr/bin/clang++-19

# USER root
# vult user
# Replace 1001 with the actual UID and GID from the host
# these values depend on your user: uid=1006(vult) gid=1007(vult) groups=1007(vult),1003(wheel)
ARG USER_ID=1006
ARG GROUP_ID=1007

# Create a group and user
RUN groupadd -g ${GROUP_ID} vult && \
    useradd -l -u ${USER_ID} -g vult -m -s /bin/bash vult && \
    install -d -m 0755 -o vult -g vult /home/vult

# Set the user to use when running the image
USER vult

# Set the working directory
WORKDIR /home/vult/


#  install depot_tools+AFLplusplus outside of the docker
# install depot_tools
# Conditional git clone if the directory is empty
RUN mkdir -p /home/vult/depot_tools && \
    [ "$(ls -A /home/vult/depot_tools)" ] || git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git /home/vult/depot_tools
ENV PATH="/home/vult/depot_tools:${PATH}"

# install AFLplusplus
# Conditional git clone if the directory is empty
# AFLplusplus version 4.10a
RUN mkdir -p /home/vult/AFLplusplus && \
    [ "$(ls -A /home/vult/AFLplusplus)" ] || (\
    git clone https://github.com/AFLplusplus/AFLplusplus.git /home/vult/AFLplusplus && \
    cd /home/vult/AFLplusplus && \
    git checkout ca0c9f6d1797bac121996c3b2ac50423f6e67b8f)


ENV PATH="/home/vult/AFLplusplus::${PATH}"
ENV PATH="/usr/lib/llvm-19/bin:${PATH}"


# Install oh-my-zsh
RUN sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended

# Install zsh-autosuggestions
RUN git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions

# Install zsh-syntax-highlighting
RUN git clone https://github.com/zsh-users/zsh-syntax-highlighting.git ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting

# Install zsh-completions
RUN git clone https://github.com/zsh-users/zsh-completions ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-completions

# Set up zshrc config
RUN echo "source \${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions/zsh-autosuggestions.zsh" >> ~/.zshrc \
    && echo "source \${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh" >> ~/.zshrc \
    && echo "source \${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-completions/zsh-completions.plugin.zsh" >> ~/.zshrc
    


# Set zsh as default shell
ENV SHELL /bin/zsh



CMD ["/bin/zsh"]

# ======================== BUILDING ========================
# sudo docker compose build && sudo docker compose run v8 zsh