FROM gcc:13-bookworm

RUN apt-get update && apt-get install -y \
    cmake \
    git \
    libsdl2-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone and install FTXUI from GitHub (doesn't need a local FTXUI-main folder)
RUN git clone --depth 1 https://github.com/ArthurSonzogni/FTXUI /tmp/ftxui && \
    cmake -S /tmp/ftxui -B /tmp/ftxui/build -DFTXUI_BUILD_EXAMPLES=OFF -DFTXUI_BUILD_DOCS=OFF && \
    cmake --build /tmp/ftxui/build -j$(nproc) && \
    cmake --install /tmp/ftxui/build && \
    rm -rf /tmp/ftxui

ENV SDL_AUDIODRIVER=dummy
WORKDIR /app

# Compile your code on run and forward CLI arguments
ENTRYPOINT ["sh", "-c", "mkdir -p build && g++ -g -std=c++20 *.cpp -o build/app -lftxui-component -lftxui-dom -lftxui-screen -lSDL2 && ./build/app \"$@\"", "--"]
