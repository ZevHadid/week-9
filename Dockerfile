# Use Debian Bookworm with GCC/g++ pre-installed
FROM gcc:13-bookworm

# Install CMake, Git, and SDL2 development libraries
RUN apt-get update && apt-get install -y \
    cmake \
    git \
    libsdl2-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone, build, and install FTXUI system-wide inside the container
RUN git clone --depth 1 https://github.com /tmp/ftxui && \
    cmake -S /tmp/ftxui -B /tmp/ftxui/build -DFTXUI_BUILD_EXAMPLES=OFF -DFTXUI_BUILD_DOCS=OFF && \
    cmake --build /tmp/ftxui/build -j$(nproc) && \
    cmake --install /tmp/ftxui/build && \
    rm -rf /tmp/ftxui

# Prevent SDL2 from throwing audio errors inside a headless container
ENV SDL_AUDIODRIVER=dummy

# Set working directory
WORKDIR /app

# Create build folder, compile using relative paths, and run the app
CMD ["sh", "-c", "mkdir -p build && g++ -g -std=c++20 *.cpp -o build/app -lftxui-component -lftxui-dom -lftxui-screen -lSDL2 && ./build/app"]
