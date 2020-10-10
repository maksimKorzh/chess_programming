#
# Web based GUI for BBC chess engine
#

# packages
from flask import Flask
from flask import render_template
from flask import request
import chess
import chess.engine

# create chess engine instance
engine = chess.engine.SimpleEngine.popen_uci('./engine/bbc_1.2')

# create web app instance
app = Flask(__name__)

# root(index) route
@app.route('/')
def root():
    return render_template('bbc.html')

# make move API
@app.route('/make_move', methods=['POST'])
def make_move():
    # extract FEN string from HTTP POST request body
    fen = request.form.get('fen')

    # init python chess board instance
    board = chess.Board(fen)
    
    # search for best move
    result = engine.play(board, chess.engine.Limit(time=0.1))
    
    # update internal python chess board state
    board.push(result.move)
    
    # extract FEN from current board state
    fen = board.fen()
    
    return {'fen': fen, 'best_move': str(result.move)}

# main driver
if __name__ == '__main__':
    # start HTTP server
    app.run(debug=True, threaded=True)
