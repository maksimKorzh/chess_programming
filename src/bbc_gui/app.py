#
# Web based GUI for BBC chess engine
#

# packages
from flask import Flask
from flask import render_template

# create web app instance
app = Flask(__name__)

# define root(index) route
@app.route('/')
def root():
    return render_template('bbc.html')

# main driver
if __name__ == '__main__':
    # start HTTP server
    app.run(debug=True, threaded=True)
