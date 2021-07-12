
#include <iostream>
#include <vector>
#include <string>

#define TERM_F(k,h,i,j) "F" << (k) << "[" << (h) << "," << (i) << "," << (j) << "]"
#define TERM_X(h,i,j) "X[" << (h) << "," << (i) << "," << (j) << "]"
#define CONSTRAINT_A(k,h,i,j) "CONSTRAINT_A" << (k) << "[" << (h) << "," << (i) << "," << (j) << "]"
#define CONSTRAINT_V(k,h,i) "CONSTRAINT_V" << (k) << "[" << (h) << "," << (i) << "]"

void generate_mstp(const std::vector<std::vector<size_t> >& cost) {
  const size_t N = cost.size();
  std::cout << "NAME MINIMUM-SPANNING-TREE-PROBLEM" << std::endl;
  std::cout << "ROWS" << std::endl;
  std::cout << " N OBJECTIVES" << std::endl;
  for (size_t k = 0; k < N; ++k) {
    for (size_t i = 0; i < N; ++i) {
      std::cout << " E " CONSTRAINT_V(k, 0, i) << std::endl;
    }
  }
  for (size_t k = 0; k < N; ++k) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        std::cout << " G " CONSTRAINT_A(k, 0, i, j) << std::endl;
      }
    }
  }
  std::cout << "COLUMNS" << std::endl;
  std::cout << " GVANF 'MARKER' 'INTORG'" << std::endl;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      std::cout << " " TERM_X(0, i, j) " OBJECTIVES " << cost[i][j] << std::endl;
      for (size_t k = 0; k < N; ++k) {
        std::cout << " " TERM_X(0, i, j) " " CONSTRAINT_A(k, 0, i, j)  " 1" << std::endl;
      }
    }
  }
  std::cout << " GVEND 'MARKER' 'INTEND'" << std::endl;
  for (size_t k = 0; k < N; ++k) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        std::cout << " " TERM_F(k, 0, i, j) " " CONSTRAINT_A(k, 0, i, j) " -1" << std::endl;
        if (i != j) {
          std::cout << " " TERM_F(k, 0, i, j) " " CONSTRAINT_V(k, 0, i) " 1 " CONSTRAINT_V(k, 0, j) " -1" << std::endl;
        } else {
          std::cout << " " TERM_F(k, 0, i, i) " " CONSTRAINT_V(k, 0, i) " -1" << std::endl;
        }
      }
    }
  }
  std::cout << "RHS" << std::endl;
  for (size_t k = 0; k < N; ++k) {
    std::cout << " RHS " CONSTRAINT_V(k, 0, k) " -1" << std::endl;
  }
  std::cout << "BOUNDS" << std::endl;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      std::cout << " UP BOUND " TERM_X(0, i, j) " 1" << std::endl;
    }
  }
  for (size_t k = 0; k < N; ++k) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        std::cout << " UP BOUND " TERM_F(k, 0, i, j) " 1" << std::endl;
      }
    }
  }
  std::cout << "ENDATA" << std::endl;
}

void generate_hmstp(const std::vector<std::vector<size_t> >& cost, size_t H) {
  const size_t N = cost.size();
  std::cout << "NAME HOP-CONSTRAINED-MINIMUM-SPANNING-TREE-PROBLEM" << std::endl;
  std::cout << "ROWS" << std::endl;
  std::cout << " N OBJECTIVES" << std::endl;
  for (size_t k = 0; k < N; ++k) {
    for (size_t h = 0; h < H; ++h) {
      for (size_t i = 0; i < N; ++i) {
        std::cout << " E " CONSTRAINT_V(k, h, i) << std::endl;
      }
    }
  }
  for (size_t k = 0; k < N; ++k) {
    for (size_t h = 0; h < H; ++h) {
      if (h == 0) {
        for (size_t i = 0; i < N; ++i) {
          std::cout << " G " CONSTRAINT_A(k, h, i, i) << std::endl;
        }
      } else {
        for (size_t i = 0; i < N; ++i) {
          for (size_t j = 0; j < N; ++j) {
            if (i != j) {
              std::cout << " G " CONSTRAINT_A(k, h, i, j) << std::endl;
            }
          }
        }
      }
    }
  }
  std::cout << "COLUMNS" << std::endl;
  std::cout << " GVANF 'MARKER' 'INTORG'" << std::endl;
  for (size_t h = 0; h < H; ++h) {
    if (h == 0) {
      for (size_t i = 0; i < N; ++i) {
        std::cout << " " TERM_X(h, i, i) " OBJECTIVES " << cost[i][i] << std::endl;
        for (size_t k = 0; k < N; ++k) {
          std::cout << " " TERM_X(h, i, i) " " CONSTRAINT_A(k, h, i, i)  " 1" << std::endl;
        }
      }
    } else {
      for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
          if (i != j) {
            std::cout << " " TERM_X(h, i, j) " OBJECTIVES " << cost[i][j] << std::endl;
            for (size_t k = 0; k < N; ++k) {
              std::cout << " " TERM_X(h, i, j) " " CONSTRAINT_A(k, h, i, j)  " 1" << std::endl;
            }
          }
        }
      }
    }
  }
  std::cout << " GVEND 'MARKER' 'INTEND'" << std::endl;
  for (size_t k = 0; k < N; ++k) {
    for (size_t h = 0; h < H; ++h) {
      if (h == 0) {
        for (size_t i = 0; i < N; ++i) {
          std::cout << " " TERM_F(k, h, i, i) " " CONSTRAINT_V(k, h, i) " -1" << std::endl;
          std::cout << " " TERM_F(k, h, i, i) " " CONSTRAINT_A(k, h, i, i) " -1" << std::endl;
        }
      } else {
        for (size_t i = 0; i < N; ++i) {
          for (size_t j = 0; j < N; ++j) {
            if (i != j) {
              std::cout << " " TERM_F(k, h, i, j) " " CONSTRAINT_V(k, h - 1, i) " 1 " CONSTRAINT_V(k, h, j) " -1" << std::endl;
              std::cout << " " TERM_F(k, h, i, j) " " CONSTRAINT_A(k, h, i, j) " -1" << std::endl;
            } else {
              std::cout << " " TERM_F(k, h, i, i) " " CONSTRAINT_V(k, h - 1, i) " 1 " CONSTRAINT_V(k, H - 1, i) " -1" << std::endl;
            }
          }
        }
      }
    }
  }
  std::cout << "RHS" << std::endl;
  for (size_t k = 0; k < N; ++k) {
    std::cout << " RHS " CONSTRAINT_V(k, H - 1, k) " -1" << std::endl;
  }
  std::cout << "BOUNDS" << std::endl;
  for (size_t h = 0; h < H; ++h) {
    if (h == 0) {
      for (size_t i = 0; i < N; ++i) {
        std::cout << " UP BOUND " TERM_X(h, i, i) " 1" << std::endl;
      }
    } else {
      for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
          if (i != j) {
            std::cout << " UP BOUND " TERM_X(h, i, j) " 1" << std::endl;
          }
        }
      }
    }
  }
  for (size_t k = 0; k < N; ++k) {
    for (size_t h = 0; h < H; ++h) {
      if (h == 0) {
        for (size_t i = 0; i < N; ++i) {
          std::cout << " UP BOUND " TERM_F(k, h, i, i) " 1" << std::endl;
        }
      } else {
        for (size_t i = 0; i < N; ++i) {
          for (size_t j = 0; j < N; ++j) {
            std::cout << " UP BOUND " TERM_F(k, h, i, j) " 1" << std::endl;
          }
        }
      }
    }
  }
  std::cout << "ENDATA" << std::endl;
}

int main(int argc, char** argv) {
  size_t H = 2;
  if (argc >= 2) {
    char* end;
    size_t arg = strtoul(argv[1], &end, 10);
    if (end > argv[1]) {
      H = arg;
    } else {
      std::cout << "usage: formulate [H] < matrix.txt > output.mps" << std::endl;
      return 0;
    }
  }
  size_t N;
  std::cin >> N;
  std::string line;
  for (size_t i = 0; i < N; ++i) {
    std::cin >> line;
  }
  std::vector<std::vector<size_t> > cost(N);
  for (auto& row : cost) {
    row.resize(N);
    for (auto& cell : row) {
      std::cin >> cell;
    }
  }
  if (H >= 1) {
    generate_hmstp(cost, H);
  } else {
    generate_mstp(cost);
  }
  return 0;
}
