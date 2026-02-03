{
  description = "OpenGL Sprite Test";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    jn = {
      url = "github:Just-Natsuki-Team/NatsukiModDev";
      flake = false;
    };
  };

  outputs = {
    self,
    jn,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};

        sdl3-src = pkgs.fetchFromGitHub {
          owner = "libsdl-org";
          repo = "SDL";
          rev = "release-3.2.14";
          hash = "sha256-+CcbvF1nxxsVwuO5g50sBVGth0sr5WTFojSfT6B6bok=";
        };
      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "opengl_test";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            git
          ];

          buildInputs = with pkgs; [
            libGL
            libGLU

            wayland
            wayland-protocols
            libxkbcommon
            libdecor

            vulkan-headers
            vulkan-loader

            xorg.libX11
            xorg.libXext
            xorg.libXcursor
            xorg.libXi
            xorg.libXrandr
            xorg.libXScrnSaver
            xorg.libXinerama
            xorg.libXfixes

            sdl3
          ];

          cmakeFlags = [
            "-DUSE_SYSTEM_SDL3=ON"
            "-DMOD_ASSETS_SOURCE=${jn}/game/mod_assets"
          ];

          postInstall = ''
            patchelf --add-rpath ${pkgs.libGL}/lib $out/bin/opengl_test
            patchelf --add-rpath ${pkgs.wayland}/lib $out/bin/opengl_test
          '';
        };

        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [cmake ninja pkg-config git];
          buildInputs = with pkgs; [
            libGL
            wayland
            wayland-protocols
            libxkbcommon
          ];

          shellHook = ''
            export CMAKE_PREFIX_PATH=${pkgs.libGL}:${pkgs.wayland}:$CMAKE_PREFIX_PATH
          '';
        };
      }
    );
}
