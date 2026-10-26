{ lib
, llvmPackages
, cmake
, ninja
, doctest
}:

llvmPackages.stdenv.mkDerivation rec {
  pname = "entity";
  version = "0.1.0";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
    ninja
  ];

  buildInputs = with pkgs; [
    doctest
  ];
}
