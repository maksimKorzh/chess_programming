##########################################
#
#   3 nested loops move generator draft
#   based on "fen" board representation
#
#                   by
#
#            Code Monkey King
#
##########################################

#fen = 'rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1'
fen = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 '
board = list(('x' + 'x' * len(fen.split('/')[0]) + 'x\n') * 2 + 'x' + ''.join([
    '.' * int(c) if c.isdigit() else c for c in fen.split()[0].replace('/', 'x\nx')
]) + 'x\n' + ('x' + 'x' * len(fen.split('/')[0]) + 'x\n') * 2 + fen.split()[1])
N, S, E, W = -len(fen.split('/')[0]) - 3, len(fen.split('/')[0]) + 3, 1, -1
moves = {'P': [N, N + E, N + W], 'p': [S, S + E, S + W],
         'n': [2*N + W, 2*S + E, E + 2*N, W + 2*S],}

for square in range(len(board) - 1):
    piece = board[square]
    if piece not in 'x\n.':
        if (piece.isupper() if board[-1] == 'w' else piece.islower()):
            for key, direction in moves.items():
                piece_type = board[square].lower() if board[square] != 'P' else board[square]
                if piece_type == key:
                    for offset in direction:
                        target_square = square
                        while True:
                            target_square += offset
                            captured_piece = board[target_square]
                            if captured_piece == 'x': break
                            if (captured_piece.isupper() if board[-1] == 'w' else captured_piece.islower()): break
                            if piece_type in 'Pp' and abs(offset) == 11 and captured_piece != '.': break
                            if piece_type in 'Pp' and abs(offset) in [S + E,  S + W] and captured_piece == '.': break
                            board[target_square] = board[square]
                            board[square] = '.'
                            print(''.join(board)); input()
                            board[square] = piece
                            board[target_square] = captured_piece
                            print(''.join(board)); input()
                            if piece_type in 'Ppnk': break
                            if captured_piece != '.': break




