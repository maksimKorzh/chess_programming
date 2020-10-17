// in MakeMove():
piece = board[from];
victim = board[to];
board[from] = EMPTY;
board[to] = piece;

for(i=0; i<256; i++) {                 // for all cells in layer 1
  if(IsKing(piece)) {
    int sum=0;
    king[stm] = to;                    // New king location; stm = 0 (white) or 1 (black)
    for(s=0; s<64; s++) {              // for all board squares
      int p = board[s];                // get the piece on it
      sum += KPST[to][p][s];           // add its KPST score to the total
    }
    layer1[i + 256*stm] = sum;         // this is the board's PST eval for the new King location
  } else {                             // other than King, update incrementally
    for(j=0; j<2; j++) {               // for both players
      int k = king[j];                 // get their King's location   
      layer1[i+256*j] -= KPST[k][piece][from]; // update the PST eval incrementally
      layer1[i+256*j] += KPST[k][piece][to];
      layer1[i+256*j] -= KPST[k][victim][to];
    }
  }
}

for(i=0; i<32; i++) {                  // for all cells in layer 2
  int sum = 0;                         // prepare calculating their input for cell i
  for(j=0; j<512; j++) {               // for each of the cells in layer 1
    sum += weights1[i][j] * layer1[j]; // add its weighted output to the input
  }                                    // we now have the total input of cell i in layer 2
  layer2[i] = max(0, sum);             // calculate its output
}

for(i=0; i<32; i++) {                  // for all cells in layer 3
  int sum = 0;                         // prepare calculating their input for cell i
  for(j=0; j<32; j++) {                // for each of the cells in layer 2
    sum += weights2[i][j] * layer2[j]; // add its weighted output to the input
  }                                    // we now have the total input of cell i in layer 3
  layer3[i] = max(0, sum);             // calculate its output
}

int eval = 0;                          // prepare calculating the evaluation score
for(j=0; j<32; j++) {                  // for each of the cells in layer 3
  sum += weights3[j] * layer3[j];      // add its weighted output to the input
}                                      // we now have the total input the final cell
                                       // which just outputs it unmodified
