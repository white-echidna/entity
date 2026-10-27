{
  lib,
  llvmPackages,
  cmake,
  ninja,
  doctest,
  glm,
}:

llvmPackages.libcxxStdenv.mkDerivation {
  pname = "entity";
  version = "0.1.0";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
    ninja
  ];

  buildInputs = [
    doctest
    glm
  ];
}
