function benchBoardLoop() {
    const OUT_OF_BOARD_MASK = 0x88;

    for (let run = 0; run < 5; run++) {
        console.time('bench 1');
        let count1 = 0;
        for (let i = 0; i < 100000000; i++) {
            for (let square = 0; square < 120; square++) {
                if (square & OUT_OF_BOARD_MASK) continue;
                if (square == 68) count1++;
            }
        }
        console.log(count1);
        console.timeEnd('bench 1');

        console.time('bench 2');
        let count2 = 0;
        for (let i = 0; i < 100000000; i++) {
            for (let square = 0; square < 64; square++) {
                let square88 = ((square << 1) & 0x70) | (square & 0x07);
                if (square88 == 68) count2++;
            }
        }
        console.log(count2);
        console.timeEnd('bench 2');

        console.time('bench 3');
        let count3 = 0;
        for (let i = 0; i < 100000000; i++) {
            for (let square = 0; square < 64; square++) {
                let square88 = square + (square & 0x38);
                if (square88 == 68) count3++;
            }
        }
        console.log(count3);
        console.timeEnd('bench 3');

        console.time('bench 4');
        let count4 = 0;
        for (let i = 0; i < 100000000; i++) {
            for (let square = 0; square < 64; square++) {
                let square88 = square + (square & ~7);
                if (square88 == 68) count4++;
            }
        }
        console.log(count4);
        console.timeEnd('bench 4');
    }
}

function benchPawnStartingRankDetection() {
    const OUT_OF_BOARD_MASK = 0x88;
    const RANK_MASK = 0x70;
    const PAWN_STARTING_RANK = [0x60, 0x10];
    let ACTIVE_COLOR = 0;

    for (let run = 0; run < 5; run++) {
        console.time('bench 1');
        ACTIVE_COLOR = 0;
        let count1 = 0;
        for (let i = 0; i < 10000000; i++) {
            for (let square = 0; square < 128; square++) {
                if (square & OUT_OF_BOARD_MASK) continue;
                if ((~square & RANK_MASK) == (96 - 80 * ACTIVE_COLOR)) {
                    count1++;
                }
            }
            ACTIVE_COLOR = 1 - ACTIVE_COLOR;
        }
        console.log(count1);
        console.timeEnd('bench 1');

        console.time('bench 2');
        ACTIVE_COLOR = 0;
        let count2 = 0;
        for (let i = 0; i < 10000000; i++) {
            for (let square = 0; square < 128; square++) {
                if (square & OUT_OF_BOARD_MASK) continue;
                if ((square & RANK_MASK) == PAWN_STARTING_RANK[ACTIVE_COLOR]) {
                    count2++;
                }
            }
            ACTIVE_COLOR = 1 - ACTIVE_COLOR;
        }
        console.log(count2);
        console.timeEnd('bench 2');
    }
}

function benchBoardArray() {
    const OUT_OF_BOARD_MASK = 0x88;
    let BOARD_1 = new Uint8Array(128);
    let BOARD_2 = [];

    for (let run = 0; run < 5; run++) {
        console.time('bench 1');
        let count1 = 0;
        for (let i = 0; i < 10000000; i++) {
            for (let square = 0; square < 128; square++) {
                BOARD_1[square] = 0;
            }
            for (let square = 0; square < 128; square++) {
                BOARD_1[square] = square;
            }
            for (let square = 0; square < 128; square++) {
                if (BOARD_1[square] % 3 == 0) {
                    count1++;
                }
            }
        }
        console.log(count1);
        console.timeEnd('bench 1');

        console.time('bench 2');
        let count2 = 0;
        for (let i = 0; i < 10000000; i++) {
            for (let square = 0; square < 128; square++) {
                BOARD_2[square] = 0;
            }
            for (let square = 0; square < 128; square++) {
                BOARD_2[square] = square;
            }
            for (let square = 0; square < 128; square++) {
                if (BOARD_2[square] % 3 == 0) {
                    count2++;
                }
            }
        }
        console.log(count2);
        console.timeEnd('bench 2');
    }
}

function benchMakePiece() {

    for (let run = 0; run < 5; run++) {
        console.time('bench 1');
        let count1 = 0;
        for (let i = 0; i < 100000000; i++) {
            for (let pieceType = 1; pieceType < 6; pieceType++) {
                for (let color = 0; color <= 1; color++) {
                    let piece = pieceType | (color << 3);
                    if (piece == 14) count1++;
                }
            }
        }
        console.log(count1);
        console.timeEnd('bench 1');

        console.time('bench 2');
        let count2 = 0;
        for (let i = 0; i < 100000000; i++) {
            for (let pieceType = 1; pieceType < 6; pieceType++) {
                for (let color = 0; color <= 1; color++) {
                    let piece = pieceType + color * 8;
                    if (piece == 14) count2++;
                }
            }
        }
        console.log(count2);
        console.timeEnd('bench 2');
    }
}

benchBoardLoop();
//benchPawnStartingRankDetection();
//benchBoardArray();
//benchMakePiece();