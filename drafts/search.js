// drafts of Wukong + BBC hybrid engine search (to be honest it sucks)

  /****************************\
                 
            EVALUATION
                 
  \****************************/
  
  // piece weights
  var piece_weights = [0, 100, 300, 350, 500, 900, 1000, -100, -300, -350, -500, -900, -1000];

  // evaluate position
  function evaluate() {
    // piece counts for material draw detection
    var piece_counts = {
      [P]: 0, [N]: 0, [B]: 0, [R]: 0, [Q]: 0, [K]: 0,
      [p]: 0, [n]: 0, [b]: 0, [r]: 0, [q]: 0, [k]: 0
    }

    // static score (material + positional benefits)
    var score = 0;
    
    // loop over board squares
    for(var square = 0; square < 128; square++)
    {
        // make sure square is on board
        if(!(square & 0x88))
        {
            // init piece
            var piece = board[square]
            
            // if piece available
            if(piece)
            {
                // count piece
                piece_counts[piece]++;
                
                // calculate material score
                score += piece_weights[piece];
                
                // white positional score
                if (piece >= P && piece <= K)
                  // calculate positional score
                  score += board[square + 8];
                  
                // black positional score
                else if (piece >= p && piece <= k)
                  // calculate positional score
                  score -= board[square + 8];
            }
        }
    }
    
    // no pawns on board (material draw detection)
    if(!piece_counts[P] && !piece_counts[p]) {
      // no rooks & queens on board
      if (!piece_counts[R] && !piece_counts[r] && !piece_counts[Q] && !piece_counts[q]) {
	      // no bishops on board
	      if (!piece_counts[B] && !piece_counts[b]) {
	        // less than 3 knights on board for either side
          if (piece_counts[N] < 3 && piece_counts[n] < 3)
            // return material draw
            return 0;
	      }
	      
	      // no knights on board
	      else if (!piece_counts[N] && !piece_counts[n]) {
          // less than 2 bishops on board for both sides
          if (Math.abs(piece_counts[B] - piece_counts[b]) < 2)
            // return material draw
            return 0;
	      }
	      
	      // less than 3 white knights and no white bishops or 1 white bishop and no white knights
	      else if ((piece_counts[N] < 3 && !piece_counts[B]) || (piece_counts[B] == 1 && !piece_counts[N])) {
	        // same as above but for black
	        if ((piece_counts[n] < 3 && !piece_counts[b]) || (piece_counts[b] == 1 && !piece_counts[n]))
	          // return material draw
	          return 0;
	      }
	      
	    }
	    
	    // no queens on board
	    else if (!piece_counts[Q] && !piece_counts[q]) {
        // each side has one rook
        if (piece_counts[R] == 1 && piece_counts[r] == 1) {
          // each side has less than two minor pieces
          if ((piece_counts[N] + piece_counts[B]) < 2 && (piece_counts[n] + piece_counts[b]) < 2)
            // return material draw
            return 0;
        }
        
        // white has one rook and no black rooks
        else if (piece_counts[R] == 1 && !piece_counts[r]) {        
          // white has no minor pieces and black has either one or two minor pieces
          if ((piece_counts[N] + piece_counts[B] == 0) &&
            (((piece_counts[n] + piece_counts[b]) == 1) || 
             ((piece_counts[n] + piece_counts[b]) == 2)))
            // return material draw
            return 0;

        }
        
        // black has one rook and no white rooks
        else if (piece_counts[r] == 1 && !piece_counts[R]) {
          // black has no minor pieces and white has either one or two minor pieces
          if ((piece_counts[n] + piece_counts[b] == 0) &&
            (((piece_counts[N] + piece_counts[B]) == 1) ||
             ((piece_counts[N] + piece_counts[B]) == 2)))
            // return material draw
            return 0;
        }
      }
    }
    
    // return score depending on side to move
    return (side == white) ? score : -score;
  }
  
  
  /****************************\
                 
        TRANSPOSITION TABLE
                 
  \****************************/

  // number hash table entries
  var hash_entries = 2796202;

  // no hash entry found constant
  const no_hash_entry = 100000;

  // transposition table hash flags
  const hash_flag_exact = 0;
  const hash_flag_alpha = 1;
  const hash_flag_beta = 2;

  // define TT instance
  var hash_table = [];
  
  // clear TT (hash table)
  function clear_hash_table() {
    // loop over TT elements
    for (var index = 0; index < hash_entries; index++) {
      // reset TT inner fields
      hash_table[index] = {
        hash_key: 0,
        depth: 0,
        flag: 0,
        score: 0,
        best_move: 0
      }
    }
  }
  
  // read hash entry data
  function read_hash_entry(alpha, beta, best_move, depth) {
    var hash_entry = hash_table[Math.abs(hash_key) % hash_entries];

    // make sure we're dealing with the exact position we need
    if (hash_entry.hash_key == hash_key) {
      // make sure that we match the exact depth our search is now at
      if (hash_entry.depth >= depth) {
        // extract stored score from TT entry
        var score = hash_entry.score;
        
        // retrieve score independent from the actual path
        // from root node (position) to current node (position)
        if (score < -mate_score) score += ply;
        if (score > mate_score) score -= ply;
    
        // match the exact (PV node) score 
        if (hash_entry.flag == hash_flag_exact)
          // return exact (PV node) score
          return score;
        
        // match alpha (fail-low node) score
        if ((hash_entry.flag == hash_flag_alpha) &&
            (score <= alpha))
          // return alpha (fail-low node) score
          return alpha;
        
        // match beta (fail-high node) score
        if ((hash_entry.flag == hash_flag_beta) &&
            (score >= beta))
          // return beta (fail-high node) score
          return beta;
      }

      // store best move
      best_move.value = hash_entry.best_move;
    }
    
    // if hash entry doesn't exist
    return no_hash_entry;
  }

  // write hash entry data
  function write_hash_entry(score, best_move, depth, hash_flag) {
    
    var hash_entry = hash_table[Math.abs(hash_key) % hash_entries];

    // store score independent from the actual path
    // from root node (position) to current node (position)
    if (score < -mate_score) score -= ply;
    if (score > mate_score) score += ply;

    // write hash entry data 
    hash_entry.hash_key = hash_key;
    hash_entry.score = score;
    hash_entry.flag = hash_flag;
    hash_entry.depth = depth;
    hash_entry.best_move = best_move;
  }
  
  /****************************\
                 
              SEARCH
                 
  \****************************/
  
  // variable to flag that time is up
  var stopped = 0;
  
  /* 
       These are the score bounds for the range of the mating scores
     [-infinity, -mate_value ... -mate_score, ... score ... mate_score ... mate_value, infinity]
  */
     
  const infinity = 50000;
  const mate_value = 49000;
  const mate_score = 48000;

  // most valuable victim & less valuable attacker

  /*
                            
      (Victims) Pawn Knight Bishop   Rook  Queen   King
    (Attackers)
          Pawn   105    205    305    405    505    605
        Knight   104    204    304    404    504    604
        Bishop   103    203    303    403    503    603
          Rook   102    202    302    402    502    602
         Queen   101    201    301    401    501    601
          King   100    200    300    400    500    600
  */

  // MVV LVA
  var mvv_lva = [
	  0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   0,   0,
	  0, 105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	  0, 104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	  0, 103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	  0, 102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	  0, 101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	  0, 100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	  0, 105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	  0, 104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	  0, 103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	  0, 102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	  0, 101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	  0, 100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
  ];

  // max ply that we can reach within a search
  const max_ply = 64;

  // killer moves
  var killer_moves = new Array(2 * max_ply);

  // history moves
  var history_moves = new Array(13 * 128);

  /*
        ================================
              Triangular PV table
        --------------------------------
          PV line: e2e4 e7e5 g1f3 b8c6
        ================================
             0    1    2    3    4    5
        
        0    m1   m2   m3   m4   m5   m6
        
        1    0    m2   m3   m4   m5   m6 
        
        2    0    0    m3   m4   m5   m6
        
        3    0    0    0    m4   m5   m6
         
        4    0    0    0    0    m5   m6
        
        5    0    0    0    0    0    m6
  */

  // PV length
  var pv_length = new Array(max_ply);

  // PV table
  var pv_table = new Array(max_ply * max_ply);

  // follow PV & score PV move
  var follow_pv;
  var score_pv;
  
  // repetition table
  var repetition_table = new Array(1000);
  
  // repetition index
  var repetition_index = 0;

  // half move counter
  var ply = 0;
  
  // clear search data structures
  function clear_search() {
    // reset nodes counter
    nodes = 0;
    
    // reset "time is up" flag
    stopped = 0;
    
    // reset follow PV flags
    follow_pv = 0;
    score_pv = 0;
    
    // clear helper data structures for search
    for (var index = 0; index < killer_moves.length; index++) killer_moves[index] = 0;
    for (var index = 0; index < history_moves.length; index++) history_moves[index] = 0;
    for (var index = 0; index < pv_table.length; index++) pv_table[index] = 0;
    for (var index = 0; index < pv_length.length; index++) pv_length[index] = 0;
  }

  // enable PV move scoring
  function enable_pv_scoring(move_list) {
    // disable following PV
    follow_pv = 0;
    
    // loop over the moves within a move list
    for (var count = 0; count < move_list.count; count++) {
      // make sure we hit PV move
      if (pv_table[ply] == move_list.moves[count]) {
        // enable move scoring
        score_pv = 1;
        
        // enable following PV
        follow_pv = 1;
      }
    }
  }
  
  /*  =======================
         Move ordering
    =======================
    
    1. PV move
    2. Captures in MVV/LVA
    3. 1st killer move
    4. 2nd killer move
    5. History moves
    6. Unsorted moves
  */

  // score moves
  function score_move(move) {
    // if PV move scoring is allowed
    if (score_pv) {
        // make sure we are dealing with PV move
        if (pv_table[ply] == move) {
            // disable score PV flag
            score_pv = 0;
            
            // give PV move the highest score to search it first
            return 20000;
        }
    }
      
    // score capture move
    if (get_move_capture(move))
      // score move by MVV LVA lookup
      return mvv_lva[board[get_move_source(move)] * 13 + board[get_move_target(move)]] + 10000
    
    // score quiet move
    else {
      // score 1st killer move
      if (killer_moves[ply] == move)
        return 9000;
      
      // score 2nd killer move
      else if (killer_moves[max_ply + ply] == move)
        return 8000;
      
      // score history move
      else
        return history_moves[get_move_piece(move) * 128 + get_move_target(move)];
    }
    
    return 0;
  }
  
  // sort moves in descending order
  function sort_moves(move_list, best_move) {
    // move scores
    var move_scores = new Array(move_list.count);
    
    // score all the moves within a move list
    for (var count = 0; count < move_list.count; count++) {
        // if hash move available
        if (best_move == move_list.moves[count])
          // score move
          move_scores[count] = 30000;

        else
          // score move
          move_scores[count] = score_move(move_list.moves[count]);
    }
    
    // loop over current move within a move list
    for (var current_move = 0; current_move < move_list.count; current_move++) {
      // loop over next move within a move list
      for (var next_move = current_move + 1; next_move < move_list.count; next_move++) {
        // compare current and next move scores
        if (move_scores[current_move] < move_scores[next_move]) {
          // swap scores
          var temp_score = move_scores[current_move];
          move_scores[current_move] = move_scores[next_move];
          move_scores[next_move] = temp_score;
          
          // swap moves
          var temp_move = move_list.moves[current_move];
          move_list.moves[current_move] = move_list.moves[next_move];
          move_list.moves[next_move] = temp_move;
        }
      }
    }
  }
  
  // print move scores
  function print_move_scores(move_list) {
    console.log("Move scores:\n\n");

    // loop over moves within a move list
    for (var count = 0; count < move_list.count; count++)
      console.log('move:', print_move(move_list.moves[count]),
                  '    score: ' + score_move(move_list.moves[count]));

  }
  
  // quiescence search
  function quiescence(alpha, beta) {
    // every 2047 nodes
    //if((nodes & 2047 ) == 0)
      // "listen" to the GUI/user input
	    //communicate();
  
    // increment nodes count
    nodes++;

    // we are too deep, hence there's an overflow of arrays relying on max ply constant
    if (ply > 10)
      // evaluate position
      return evaluate();

    // evaluate position
    var evaluation = evaluate();
    
    // fail-hard beta cutoff
    if (evaluation >= beta)
      // node (position) fails high
      return beta;
    
    // found a better move
    if (evaluation > alpha)
      // PV node (position)
      alpha = evaluation;
    
    // create move list instance
    var move_list = {
      moves: new Array(256),
      count: 0
    }
    
    // generate moves
    generate_moves(move_list);
    
    // sort moves
    sort_moves(move_list, 0);
    
    // loop over moves within a movelist
    for (var count = 0; count < move_list.count; count++) {
      // backup current board position
      var board_copy, king_square_copy, side_copy, enpassant_copy, castle_copy, fifty_copy, hash_copy;
      board_copy = JSON.parse(JSON.stringify(board));
      side_copy = side;
      enpassant_copy = enpassant;
      castle_copy = castle;
      hash_copy = hash_key;
      fifty_copy = fifty;
      king_square_copy = JSON.parse(JSON.stringify(king_square));
      
      // increment ply
      ply++;
      
      // increment repetition index & store hash key
      repetition_index++;
      repetition_table[repetition_index] = hash_key;

      // make sure to make only legal moves
      if (make_move(move_list.moves[count], only_captures) == 0) {
        // decrement ply
        ply--;
        
        // decrement repetition index
        repetition_index--;

        // skip to next move
        continue;
      }
      
      // score current move
      var score = -quiescence(-beta, -alpha);
      
      // decrement ply
      ply--;
      
      // decrement repetition index
      repetition_index--;

      // restore board position
      board = JSON.parse(JSON.stringify(board_copy));
      side = side_copy;
      enpassant = enpassant_copy;
      castle = castle_copy;
      hash_key = hash_copy;
      fifty = fifty_copy;
      king_square = JSON.parse(JSON.stringify(king_square_copy));
      
      // reutrn 0 if time is up
      if (stopped == 1) return 0;
      
      // found a better move
      if (score > alpha)
      {
        // PV node (position)
        alpha = score;
        
        // fail-hard beta cutoff
        if (score >= beta)
          // node (position) fails high
          return beta;
      }
    }
    
    // node (position) fails low
    return alpha;
  }

  // negamax search
  function negamax(alpha, beta, depth) {       
    // PV length
    pv_length[ply] = ply;
    
    // current move's score
    var score;
    
    // best move for TT
    var best_move = {value: 0};
    
    // define hash flag
    var hash_flag = hash_flag_alpha;
    
    // a hack by Pedro Castro to figure out whether the current node is PV node or not 
    var pv_node = beta - alpha > 1;
    
    // read hash entry if we're not in a root ply and hash entry is available
    // and current node is not a PV node
    if (ply && (score = read_hash_entry(alpha, beta, best_move, depth)) != no_hash_entry && pv_node == 0)
      // if the move has already been searched (hence has a value)
      // we just return the score for this move without searching it
      return score;
    
    // escape condition
    if  (!depth)
      // search for calm position before evaluation
      return quiescence(alpha, beta);
      //return evaluate();

    // update nodes count
    nodes++;
    
    // is king in check?
    var in_check = is_square_attacked(king_square[side], side ^ 1);
    
    // increase depth if king is in check
    if (in_check) depth++;
    
    // legal moves
    var legal_moves = 0;
    
    // get static evaluation score
	  var static_eval = evaluate();
      
    // evaluation pruning / static null move pruning
	  if (depth < 3 && !pv_node && !in_check &&  Math.abs(beta - 1) > -infinity + 100) {   
      // define evaluation margin
		  var eval_margin = 120 * depth;
		  
		  // evaluation margin substracted from static evaluation score fails high
		  if (static_eval - eval_margin >= beta)
	      // evaluation margin substracted from static evaluation score
			  return static_eval - eval_margin;
	  }

	  // null move pruning
    if (depth >= 3 && in_check == 0 && ply)
    {
      // backup current board position
      var board_copy, king_square_copy, side_copy, enpassant_copy, castle_copy, fifty_copy, hash_copy;
      board_copy = JSON.parse(JSON.stringify(board));
      side_copy = side;
      enpassant_copy = enpassant;
      castle_copy = castle;
      fifty_copy = fifty;
      hash_copy = hash_key;
      king_square_copy = JSON.parse(JSON.stringify(king_square));
      
      // increment ply
      ply++;
      
      // increment repetition index & store hash key
      repetition_index++;
      repetition_table[repetition_index] = hash_key;
      
      // hash enpassant if available
      if (enpassant != no_sq) hash_key ^= piece_keys[enpassant];
      
      // reset enpassant capture square
      enpassant = no_sq;
      
      // switch the side, literally giving opponent an extra move to make
      side ^= 1;
      
      // hash the side
      hash_key ^= side_key;
              
      // search moves with reduced depth to find beta cutoffs
      score = -negamax(-beta, -beta + 1, depth - 1 - 2);

      // decrement ply
      ply--;
      
      // decrement repetition index
      repetition_index--;
          
      // restore board position
      board = JSON.parse(JSON.stringify(board_copy));
      side = side_copy;
      enpassant = enpassant_copy;
      castle = castle_copy;
      hash_key = hash_copy;
      fifty = fifty_copy;
      king_square = JSON.parse(JSON.stringify(king_square_copy));

      // reutrn 0 if time is up
      if (stopped == 1) return 0;

      // fail-hard beta cutoff
      if (score >= beta)
        // node (position) fails high
        return beta;
    }
    
	  // razoring
    if (!pv_node && !in_check && depth <= 3) {
      // get static eval and add first bonus
      score = static_eval + 125;
      
      // define new score
      var new_score;
      
      // static evaluation indicates a fail-low node
      if (score < beta) {
        // on depth 1
        if (depth == 1) {
          // get quiscence score
          new_score = quiescence(alpha, beta);
          
          // return quiescence score if it's greater then static evaluation score
          return (new_score > score) ? new_score : score;
        }
        
        // add second bonus to static evaluation
        score += 175;
        
        // static evaluation indicates a fail-low node
        if (score < beta && depth <= 2) {
          // get quiscence score
          new_score = quiescence(alpha, beta);
          
          // quiescence score indicates fail-low node
          if (new_score < beta)
            // return quiescence score if it's greater then static evaluation score
            return (new_score > score) ? new_score : score;
        }
      }
	  }
    
    // create move list variable
    var move_list = {
      moves: new Array(256),
      count: 0
    }
    
    // generate moves
    generate_moves(move_list);
    
    // if we are now following PV line
    if (follow_pv)
      // enable PV move scoring
      enable_pv_scoring(move_list);
        
    // move ordering
    sort_moves(move_list, best_move.value);
    
    // number of moves searched in a move list
    var moves_searched = 0;
    
    // loop over the generated moves
    for (var count = 0; count < move_list.count; count++)
    {
      // backup current board position
      var board_copy, king_square_copy, side_copy, enpassant_copy, castle_copy, fifty_copy, hash_copy;
      board_copy = JSON.parse(JSON.stringify(board));
      side_copy = side;
      enpassant_copy = enpassant;
      castle_copy = castle;
      fifty_copy = fifty;
      hash_copy = hash_key;
      king_square_copy = JSON.parse(JSON.stringify(king_square));
        
      // increment ply
      ply++;
      
      // increment repetition index & store hash key
      repetition_index++;
      repetition_table[repetition_index] = hash_key;
      
      // make only legal moves
      if (!make_move(move_list.moves[count], all_moves)) {
        // decrement ply
        ply--;
        
        // decrement repetition index
        repetition_index--;
        
        // skip illegal move
        continue;
      }
       
      // increment legal moves
      legal_moves++;
      
      // full depth search
      if (moves_searched == 0)
        // do normal alpha beta search
        score = -negamax(-beta, -alpha, depth - 1);
      
      // late move reduction (LMR)
      else {
        // condition to consider LMR
        if(
            moves_searched >= 4 &&
            depth >= 3 &&
            in_check == 0 && 
            get_move_capture(move_list.moves[count]) == 0 &&
            get_move_piece(move_list.moves[count]) == 0
          )
            // search current move with reduced depth:
            score = -negamax(-alpha - 1, -alpha, depth - 2);
        
        // hack to ensure that full-depth search is done
        else score = alpha + 1;
        
        // principle variation search PVS
        if(score > alpha)
        {
         /* Once you've found a move with a score that is between alpha and beta,
            the rest of the moves are searched with the goal of proving that they are all bad.
            It's possible to do this a bit faster than a search that worries that one
            of the remaining moves might be good. */
            score = -negamax(-alpha - 1, -alpha, depth-1);
        
         /* If the algorithm finds out that it was wrong, and that one of the
            subsequent moves was better than the first PV move, it has to search again,
            in the normal alpha-beta manner.  This happens sometimes, and it's a waste of time,
            but generally not often enough to counteract the savings gained from doing the
            "bad move proof" search referred to earlier. */
            if((score > alpha) && (score < beta))
             /* re-search the move that has failed to be proved to be bad
                with normal alpha beta score bounds*/
                score = -negamax(-beta, -alpha, depth-1);
        }
      }
      
      // decrement ply
      ply--;
      
      // decrement repetition index
      repetition_index--;

      // restore board position
      board = JSON.parse(JSON.stringify(board_copy));
      side = side_copy;
      enpassant = enpassant_copy;
      castle = castle_copy;
      hash_key = hash_copy;
      fifty = fifty_copy;
      king_square = JSON.parse(JSON.stringify(king_square_copy));
      
      // increment the counter of moves searched so far
      moves_searched++;
        
      // alpha acts like max in MiniMax
      if (score > alpha) {
        // switch hash flag from storing score for fail-low node
        // to the one storing score for PV node
        hash_flag = hash_flag_exact;
            
        // update history score
        history_moves[board[get_move_source(move_list.moves[count])] * 128 + get_move_target(move_list.moves[count])] += depth;

        // set alpha score
        alpha = score;
        
        // update best move
        best_move.value = move_list.moves[count];
        
        // store PV move
        pv_table[ply * 64 + ply] = move_list.moves[count];
        
        // update PV line
        for (var i = ply + 1; i < pv_length[ply + 1]; i++)
          pv_table[ply * 64 + i] = pv_table[(ply + 1) * 64 + i];
        
        // update PV length
        pv_length[ply] = pv_length[ply + 1];                
        
        //  fail hard beta-cutoff
        if (score >= beta) {
          // store hash entry with the score equal to beta
          write_hash_entry(beta, best_move.value, depth, hash_flag_beta);
          
          if (get_move_capture(move_list.moves[count]) == 0) {
            // update killer moves
            killer_moves[max_ply + ply] = killer_moves[ply];
            killer_moves[ply] = move_list.moves[count];
          }
          
          return beta;
        }
      }      
    }
    
    // if no legal moves
    if (!legal_moves) {
      // check mate detection
      if (in_check)
        return -mate_score + ply;
      
      // stalemate detection
      else
        return 0;
    }
    
    // store hash entry with the score equal to alpha
    write_hash_entry(alpha, best_move.value, depth, hash_flag);
    
    // return alpha score
    return alpha;
  }  

  // search position for the best move
  function search_position(depth) {
    // search start time
    var start = new Date().getTime();

    // define best score variable
    var score = 0;
    
    // clear search data structures
    clear_search();
        
    // define initial alpha beta bounds
    var alpha = -infinity;
    var beta = infinity;
 
    // iterative deepening
    for (var current_depth = 1; current_depth <= depth; current_depth++)
    {
      // if time is up
      if (stopped == 1)
		    // stop calculating and return best move so far 
		    break;
	  
      // enable follow PV flag
      follow_pv = 1;
      
      // find best move within a given position
      score = negamax(alpha, beta, current_depth);

      /* we fell outside the window, so try again with a full-width window (and the same depth)
      if ((score <= alpha) || (score >= beta)) {
        alpha = -infinity;    
        beta = infinity;      
        continue;
      }

      // set up the window for the next iteration
      alpha = score - 50;
      beta = score + 50;
      */
      // if PV is available
      if (pv_length[0]) {
        // print search info
        if (score > -mate_value && score < -mate_score)
          console.log('Score: mate in %d\nDepth: %d\nNodes: %d\nTime %d', -(score + mate_value) / 2 - 1, current_depth, nodes, new Date().getTime() - start);
        
        else if (score > mate_score && score < mate_value)
          console.log('Score: mate in %d\nDepth: %d\nNodes: %d\nTime %d', (mate_value - score) / 2 + 1, current_depth, nodes, new Date().getTime() - start);   
        
        else
          {}//console.log('Score: %d cp\nDepth: %d\nNodes: %d\nTime: %d', score, current_depth, nodes, new Date().getTime() - start);

        // define move string
        var pv_line = 'PV: ';
        
        // loop over the moves within a PV line
        for (var count = 0; count < pv_length[0]; count++)
          // print PV move
          pv_line += print_move(pv_table[count]) + ' ';
        
        // print new line
        console.log(pv_line);        
      }
    }
    
    // console log final info
    console.log('Score: %d cp\nDepth: %d\nNodes: %d\nTime: %d', score, depth, nodes, new Date().getTime() - start);

  }
