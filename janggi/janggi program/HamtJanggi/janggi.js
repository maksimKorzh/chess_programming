/****************************************************************************
	제작자 : 함승목 (hamt)
	제작일 : 2011.06.21. 
	설  명 : minimax 알고리즘을 사용한 장기게임 엔진
	의존성 : HMotion.js 의 HMotion클래스 인스턴스가 필요하므로, 이 파일에 의존함.
****************************************************************************/
// define
var Janggi = {
	Gung	: {No:1,Score:20000},
	Cha	: {No:2,Score:1000},
	Sang	: {No:3,Score:450},
	Ma	: {No:4,Score:700},
	Sa	: {No:5,Score:600},
	Po	: {No:6,Score:850},
	Jole	: {No:7,Score:400},

	User	: 1,
	Computer: 2
}


//위치정보
var Point = function(x, y) {
	this.x = x;
	this.y = y;
}

// 장기판
var JanggiStage = function(objName, stage_id, han_eatitems, cho_eatitems) {
	this.objName = objName;
	this.stage = document.getElementById(stage_id);
	this.width = parseInt(stage.style.width.split('px')[0]);
	this.height= parseInt(stage.style.height.split('px')[0]);
	this.x_term = this.width / 8;
	this.y_term = this.height / 9;
	this.units = [];

	this.m_depth = 4; // 내다 볼 수 뎁스

	this.tern = Janggi.User; // 1 : user, 2 : computer

	this.pos_divs = [];
	this.pos_junit_obj = null;

	this.is_final == false;
	this.com_tern = 0;

	this.is_single = true;


	this.com_beforepos = document.createElement('div');
	this.com_beforepos.className = 'position';
	this.com_beforepos.style.backgroundColor = 'red';
	this.com_beforepos.style.display = 'none';
	set_opacity(this.com_beforepos, 30);

	this.com_nowpos = document.createElement('div');
	this.com_nowpos.className = 'position';
	this.com_nowpos.style.backgroundColor = 'red';
	this.com_nowpos.style.display = 'none';
	this.motion = new HMotion(this.objName+'.motion');
	set_opacity(this.com_nowpos, 30);

	this.han_eatitems_area = document.getElementById(han_eatitems);
	this.cho_eatitems_area = document.getElementById(cho_eatitems);

	this.init = function() {
		// com
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 0, 0, Janggi.Cha), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 1, 0, Janggi.Sang), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 2, 0, Janggi.Ma), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 3, 0, Janggi.Sa), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 5, 0, Janggi.Sa), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 6, 0, Janggi.Sang), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 7, 0, Janggi.Ma), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 8, 0, Janggi.Cha), true));

		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 4, 1, Janggi.Gung), true));

		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 1, 2, Janggi.Po), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 7, 2, Janggi.Po), true));

		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 0, 3, Janggi.Jole), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 2, 3, Janggi.Jole), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 4, 3, Janggi.Jole), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 6, 3, Janggi.Jole), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 8, 3, Janggi.Jole), true));

		// user
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 0, 6, Janggi.Jole), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 2, 6, Janggi.Jole), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 4, 6, Janggi.Jole), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 6, 6, Janggi.Jole), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 8, 6, Janggi.Jole), true));

		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 1, 7, Janggi.Po), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 7, 7, Janggi.Po), true));

		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 4, 8, Janggi.Gung), true));

		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 0, 9, Janggi.Cha), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 1, 9, Janggi.Sang), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 2, 9, Janggi.Ma), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 3, 9, Janggi.Sa), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 5, 9, Janggi.Sa), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 6, 9, Janggi.Sang), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 7, 9, Janggi.Ma), true));
		this.units.push(new Units(new JanggiUnit(this.x_term, this.y_term, 8, 9, Janggi.Cha), true));

		for(var i = 0 ; i < this.units.length ; i++) {
			this.stage.appendChild(this.units[i].junit_obj.obj);
			eval("addEvent(this.units[i].junit_obj.obj, 'click', function(){"+this.objName+".showPos("+this.objName+".units["+i+"].junit_obj);});");
		}

		
		this.stage.appendChild(this.com_beforepos);
		this.stage.appendChild(this.com_nowpos);
	}
	

	this.showComBeforePos = function(f_x, f_y, t_x, t_y) {
		this.com_beforepos.style.left = (f_x * this.x_term)+'px';
		this.com_beforepos.style.top = (f_y * this.y_term)+'px';
		this.com_beforepos.style.display = 'block';

		this.com_nowpos.style.left = (t_x * this.x_term)+'px';
		this.com_nowpos.style.top = (t_y * this.y_term)+'px';
		this.com_nowpos.style.display = 'block';
	}
	this.showPos = function(junit_obj) { // 선택 장기알이 갈 수 있는 위치 보여주는 함수
		this.com_beforepos.style.display = 'none';
		this.com_nowpos.style.display = 'none';
	
		if(this.is_final == true) {
			return;
		}

		if(this.tern != junit_obj.team) {
			return;
		}
		if(this.is_single == true && junit_obj.is_computer == true) {
			return;
		}
		// 같은 알 두번클릭시 포지션만 지움. 초기화되므로 removePosition()호출 전에만 비교가능.
		if(junit_obj == this.pos_junit_obj) { 
			this.removePosition();
			return;
		}

		this.removePosition();
		var poses = junit_obj.getMoveablePos(this.getStageStatus());
		for(var i = 0 ; i < poses.length ; i++) {
			var div = document.createElement('div');
			div.className = 'position';
			div.style.left = (poses[i].x * this.x_term)+'px';
			div.style.top = (poses[i].y * this.y_term)+'px';
			eval("addEvent(div, 'click', function(){"+this.objName+".ChangeUnit("+junit_obj.axis_x+", "+junit_obj.axis_y+", "+poses[i].x+', '+poses[i].y+");})");
			set_opacity(div, 80);
			this.pos_divs.push(div);
			this.stage.appendChild(div);
		}
		this.pos_junit_obj = junit_obj;
	}
	this.removePosition = function() { // 선택 장기알이 갈 수 있는 위치 보여주고있는것 삭제
		if(this.pos_divs.length <= 0) {
			return false;
		}
		for(var i = 0 ; i < this.pos_divs.length ; i++) {
			this.pos_divs[i].parentNode.removeChild(this.pos_divs[i]);
		}
		this.pos_divs = [];
		this.pos_junit_obj = null;
		return true;
	}
	this.ChangeUnit = function(from_x, from_y, to_x, to_y, motion_after) {
		var now_stage = this.getStageStatus();
		if(now_stage[from_x][from_y] == null) {
			//alert(from_x+','+from_y+' is null');
			//return;
		} else {
			//alert(from_x+','+from_y+' is exist');
		}
		var junit_obj = now_stage[from_x][from_y].junit_obj;

		if(motion_after == null) {
			junit_obj.MoveMotion(to_x, to_y, this.motion, [from_x, from_y, to_x, to_y]);
			return;
		}
		junit_obj.Move(to_x, to_y);
		this.removePosition();
		if(now_stage[to_x][to_y] != null) {
			now_stage[to_x][to_y].is_valid = false;
			now_stage[to_x][to_y].junit_obj.obj.style.display = 'none';

			var eat_box = (this.tern == Janggi.User)? this.cho_eatitems_area : this.han_eatitems_area;
			if(eat_box) {
				if(eat_box.style.position != 'relative') {
					eat_box.style.position = 'relative';
				}
				var item = document.createElement('div');
				item.className = now_stage[to_x][to_y].junit_obj.obj.className;
				item.innerHTML = now_stage[to_x][to_y].junit_obj.obj.innerHTML;

				var eated_count = eat_box.children.length; 
				var col = (eated_count % 8) * this.x_term;
				var row = parseInt(eated_count / 8) * this.y_term;

				item.style.left = col + 'px';
				item.style.top  = row + 'px';
				item.style.margin= '0px';

				eat_box.appendChild(item);
			}

			if(now_stage[to_x][to_y].junit_obj.kind == Janggi.Gung) {
				this.doGameOver((now_stage[to_x][to_y].junit_obj.team == Janggi.User));
				return;
			}
		}

		if(junit_obj.kind != Janggi.Gung) {
			this.checkJanggun();
		}

		if(junit_obj.is_computer == true) {
			this.showComBeforePos(from_x, from_y, to_x, to_y);
			this.tern = Janggi.User;
		} else {
			this.tern = Janggi.Computer;
			if(this.is_single == true) {
/*
				var com_items  = 0;
				var user_items = 0;
				for(var i = 0 ; i < this.units.length ; i++) {
					if(this.units[i].is_valid == false) {
						continue;
					}

					if(this.units[i].junit_obj.team == Janggi.User) {
						user_items++;
					} else {
						com_items++;
					}
				}
				var min = (user_items <= com_items) ? user_items : com_items;
				var total_count = user_items + com_items;


				if(total_count == 8 && this.m_depth - this.default_depth != 2) {
					this.m_depth += 2;
				}
				if(min == 4 && this.m_depth - this.default_depth != 4) {
					this.m_depth += 2;
				}
				if(min == 2 && this.m_depth - this.default_depth != 6) {
					this.m_depth += 2;
				}
*/
				this.doMinimax(this.getStageStatus(), this.m_depth, null);
			}
		}
	}

	this.checkJanggun = function() {
		var now_stage = this.getStageStatus();

		var other_team = (this.tern == Janggi.User) ? Janggi.Computer : Janggi.User;
		var other_gung_pos = null;

		var s_row = 0;
		if(other_team == Janggi.User) {
			s_row += 7;
		}
		var max_row = s_row+2;

		for(var col = 3 ; col <= 5 ; col++) {
			for(var row = s_row ; row <= max_row ; row++) {
				if(now_stage[col][row] != null && now_stage[col][row].junit_obj.kind == Janggi.Gung) {
					other_gung_pos = new Point(col, row);
					col = 5;
					break;
				}
			}
		}

		if(other_gung_pos == null) {
			return;
		}

		for(var col = 0 ; col < 9 ; col++) {
			for(var row = 0 ; row < 10 ; row++) {
				if(now_stage[col][row] == null || now_stage[col][row].junit_obj.team != this.tern) {
					continue;
				}
				var junit_obj = now_stage[col][row].junit_obj;
				var poses = junit_obj.getMoveablePos(now_stage);
				for(var i = 0 ; i < poses.length ; i++) {
					if(now_stage[poses[i].x][poses[i].y] != null && now_stage[poses[i].x][poses[i].y].junit_obj.kind == Janggi.Gung) {
						alert('장군!');
						col =  9;
						row = 10;
						break;
					}
				}
			}
		}
	}

	this.doGameOver = function(is_comwin) {
		var msg = (is_comwin) ? '패! 좀 더 연습하세요' : '승!! 축하합니다' ;
		alert(msg);
		this.is_final = true;
	}
	this.doMinimax = function(stage, depth, cut_score) {
		if(depth == this.m_depth) {
			this.com_tern++;
			mytest(this.com_tern);
		}
		if(depth == this.m_depth && this.com_tern < 6) {
			//일반 패턴
			var direct_pattern = [];
			if(
					stage[0][3] != null && stage[0][3].junit_obj.team == Janggi.Computer && stage[0][3].junit_obj.kind == Janggi.Jole 
					&& stage[0][6] != null && stage[0][6].junit_obj.team == Janggi.User && stage[0][6].junit_obj.kind == Janggi.Jole  
			  ) { // 좌 차문열기
				direct_pattern.push([0, 3, 1, 3]); 
			}
			if(
					stage[8][3] != null && stage[8][3].junit_obj.team == Janggi.Computer && stage[8][3].junit_obj.kind == Janggi.Jole && 
					stage[8][6] != null && stage[8][6].junit_obj.team == Janggi.User && stage[8][6].junit_obj.kind == Janggi.Jole  
			  ) { // 우 차문열기
				direct_pattern.push([8, 3, 7, 3]); 
			}

			// 차문 열기 있으면 적용 !
			if(stage[0][3] != null && stage[8][3] != null && direct_pattern.length > 0) {
				var pos = direct_pattern[parseInt(Math.random()*direct_pattern.length)];
				this.ChangeUnit(pos[0], pos[1], pos[2], pos[3]); 
				//alert('차문열기');
				return;
			}
			var direct_pattern = [];
			///*

			//포 궁앞에 놓도록 말올리기, 포 놓기
			if(
				stage[4][2] == null && stage[4][3] != null && stage[4][3].junit_obj.team == Janggi.Computer
				&& ( // 정해진 수대로 진행하면 안되는 조건들
					(stage[0][3] != null && stage[0][3].junit_obj.team != Janggi.Computer)
					|| (stage[8][3] != null && stage[8][3].junit_obj.team != Janggi.Computer)
					|| (stage[0][3] == null && stage[0][6] == null)
					|| (stage[8][3] == null && stage[8][6] == null)
					|| (stage[3][3] != null && stage[3][3].junit_obj.kind == Janggi.Ma)
					|| (stage[5][3] != null && stage[5][3].junit_obj.kind == Janggi.Ma)
					|| (stage[0][7] != null && stage[0][7].junit_obj.kind == Janggi.Po)
					|| (stage[8][7] != null && stage[8][7].junit_obj.kind == Janggi.Po)
					|| (stage[4][7] != null && stage[4][7].junit_obj.kind == Janggi.Po && stage[4][6] == null)
					|| stage[0][9] == null
					|| stage[8][9] == null
				) == false
			) {
				for(var i = 0 ; i <= 8 ; i++) {
					if(stage[i][0] != null && stage[i][0].junit_obj.kind == Janggi.Ma) {
						direct_pattern.push([i, 0, ((i < 4) ? i+1 : i-1), 2]);
					}
				}

				if(direct_pattern.length == 2) {
					var pos = direct_pattern[parseInt(Math.random()*100%direct_pattern.length)];
					this.ChangeUnit(pos[0], pos[1], pos[2], pos[3]); 
					//alert('마올리기');
					return;
				}

				if(direct_pattern.length == 1) {
					var x = (direct_pattern[0][0] > 4) ? 1 : 7;
					this.ChangeUnit(x, 2, 4, 2); 
					//alert('포놓기');
					return;
				}
				
			}
			//*/
		}
		




		depth--;
		var max = (this.m_depth-1)%2; // 컴이 둘 차례. max 값 보고.
		var min = (max+1)%2; // 유저가 둘 차례. min값 보고.
		var report_type = depth % 2; 

		var tern = (report_type == max) ? Janggi.Computer : Janggi.User;


		var res_score = null;
		var res_state = null;

		var node_count = 0;

		for(var col = 0 ; col < 9 ; col++) {
			for(var row = 0 ; row < 10 ; row++) {


				if(stage[col][row] != null && stage[col][row].junit_obj.team == tern) {
					var poses = stage[col][row].junit_obj.getMoveablePos(stage);

					node_count += poses.length;

					

					for(var i = 0 ; i < poses.length ; i++) {

						var state = [col, row, poses[i].x, poses[i].y];
						if(
							stage[poses[i].x][poses[i].y] != null 
							&& stage[poses[i].x][poses[i].y].junit_obj.team != stage[col][row].junit_obj.team
							&& stage[poses[i].x][poses[i].y].junit_obj.kind == Janggi.Gung
						) { // 왕 먹었다. 더 계산할거 없이 리턴
							res_score = -100000;
							if(report_type == max) res_score = 100000;

							res_state = state;
							col = 9;
							row = 10;
							break;
						}




						var param_stage = this.getChangeStageStatus(stage, col, row, poses[i].x, poses[i].y);
						var item = param_stage[poses[i].x][poses[i].y];

						var state = [col, row, poses[i].x, poses[i].y];
						if(depth == this.m_depth-1) {
							mytest(item.junit_obj.text+'Depth : '+depth+' : '+state.join('-'))
							mytest('================================');
						}



						if(depth == 0) {
							var score = get_stage_score(param_stage, Janggi.Computer) - get_stage_score(param_stage, Janggi.User);

							if(res_score == null || (report_type == min && res_score > score) || (report_type == max && res_score < score)) {
								res_score = score;
							
								if(cut_score != null && (report_type == min && res_score <= cut_score) || (report_type == max && res_score >= cut_score)) {
									col = 9;
									row = 10;
									break;
								}
							}
							continue;
						}


						var score = this.doMinimax(param_stage, depth, res_score);
						

						if(res_score == null || (report_type == max && score > res_score) || (report_type == min && score < res_score)) {
							//if(score == res_score && Math.random()*30 < 16) {
							//	continue;
							//}
							res_score = score;
							res_state = state;
							if(cut_score != null && ((report_type == max && res_score > cut_score) || (report_type == min && res_score < cut_score))) {
								mytest('cut-dep:'+depth+', rtype : '+report_type+', cut : '+cut_score+', sco : '+res_score);
								col = 9;
								row = 10;
								break;
							}
						}

					}
				}
			}
		}

		if(depth < this.m_depth-1) {
			return res_score;
		}

		mytest(res_state[0]+', '+res_state[1]+', '+res_state[2]+', '+res_state[3]);
		this.ChangeUnit (res_state[0], res_state[1], res_state[2], res_state[3]); 

	}
	

	this.getStageStatus = function() { // 현재 장기판에 유효한 장기알만 배열로 리튼
		var result = [];
		for(var col = 0 ; col < 9 ; col++) {
			result[col] = [];
		}

		for(var i = 0 ; i < this.units.length ; i++) {
			if(this.units[i].is_valid == false)
				continue;
			result[ this.units[i].junit_obj.axis_x ][ this.units[i].junit_obj.axis_y ] = this.units[i];
		}
		return result;
	}
	this.getChangeStageStatus = function(stage, f_x, f_y, d_x, d_y) {
		var result = [];
		for(var col = 0 ; col < 9 ; col++) {
			result[col] = [];
		}

		for(var col = 0 ; col < 9 ; col++) {
			for(var row = 0 ; row < 10 ; row++) {
				result[col][row] = stage[col][row];
			}
		}

		result[d_x][d_y] = result[f_x][f_y];
		result[f_x][f_y] = null;

		return result;
	}

	//private
	function addEvent(obj, type, fn)
	{
		if (obj.addEventListener)
			obj.addEventListener(type, fn, false);
		else if (obj.attachEvent)
		{       
			obj.attachEvent("on"+type, fn);
		}
	}
	function set_opacity(obj, val)
	{
		obj.style.opacity = (val/100);
		obj.style.MozOpacity = (val/100);
		obj.style.KhtmlOpacity = (val/100);
		obj.style.filter = 'alpha(opacity='+val+')';
	}

	function mytest(str) {
		//if(str.split('-')[0] != 'cut')
			return;
		var obj = document.getElementById('my_test');
		if(obj) {
			obj.innerHTML += '<br />'+str;
		}
	}
	function get_stage_score(stage, team) {
		var score = 0;
		for(var col = 0 ; col < 9 ; col++) {
			for(var row = 0 ; row < 10 ; row++) {
				if(stage[col][row] != null && stage[col][row].junit_obj.team == team) {
					score += stage[col][row].junit_obj.score;
					//score += stage[col][row].junit_obj.getAddScore(stage);

					// 포는 왕 근처에 있을때 +200
					if(stage[col][row].junit_obj.kind == Janggi.Po) {
						if(
							(team == Janggi.User && row >= 7 && col >= 3 && col <= 5)
							|| (team == Janggi.Computer && row <= 2 && col >= 3 && col <= 5)
						) {
							score += 30;
							if(
									(team == Janggi.User && row == 7 && col == 4)
									|| (team == Janggi.Computer && row == 2 && col == 4)
							  ) {
							  	score += 10;
							}
						}
					}
				}
			}
		}

		if(team == Janggi.User) {
			if(stage[4][8] != null && stage[4][8].junit_obj.kind == Janggi.Gung) {
				score += 20;

				if(stage[3][8] != null && stage[3][8].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[5][8] != null && stage[5][8].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[3][9] != null && stage[3][9].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[5][9] != null && stage[5][9].junit_obj.kind == Janggi.Sa)
					score += 10;
			} else if(stage[4][9] != null && stage[4][9].junit_obj.kind == Janggi.Gung ) {
				score += 80;
				if(stage[4][8] != null && stage[4][8].junit_obj.kind == Janggi.Sa)
					score += 13;
				if(stage[3][8] != null && stage[3][8].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[5][8] != null && stage[5][8].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[3][9] != null && stage[3][9].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[5][9] != null && stage[5][9].junit_obj.kind == Janggi.Sa)
					score += 10;
			}
		} else {
			if(stage[4][9-8] != null && stage[4][9-8].junit_obj.kind == Janggi.Gung) {
				score += 20;

				if(stage[3][9-8] != null && stage[3][9-8].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[5][9-8] != null && stage[5][9-8].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[3][9-9] != null && stage[3][9-9].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[5][9-9] != null && stage[5][9-9].junit_obj.kind == Janggi.Sa)
					score += 10;
			} else if(stage[4][9-9] != null && stage[4][9-9].junit_obj.kind == Janggi.Gung ) {
				score += 8;
				if(stage[4][9-8] != null && stage[4][9-8].junit_obj.kind == Janggi.Sa)
					score += 13;
				if(stage[3][9-8] != null && stage[3][9-8].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[5][9-8] != null && stage[5][9-8].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[3][9-9] != null && stage[3][9-9].junit_obj.kind == Janggi.Sa)
					score += 10;
				if(stage[5][9-9] != null && stage[5][9-9].junit_obj.kind == Janggi.Sa)
					score += 10;
			}
		}

		return score;
	}
	function get_max(arr) {
		if(arr.length == 0) return null;
		var max = null;
		for(var i = 0 ; i < arr.length ; i++) {
			if(max == null || arr[i] > max) {
				max = arr[i];
			}
		}
		return max;
	}
	function get_min(arr) {
		if(arr.length == 0) return null;
		var min = null;
		for(var i = 0 ; i < arr.length ; i++) {
			if(min == null || arr[i] < min) {
				min = arr[i];
			}
		}
		return min;
		
	}
}
// 장기알
var JanggiUnit = function(x_term, y_term, axis_x, axis_y, janggi_kind) {


	this.x_term = x_term;
	this.y_term = y_term;

	this.axis_x = axis_x;
	this.axis_y = axis_y;

	this.px_x = axis_x * x_term;
	this.px_y = axis_y * y_term;

	this.is_computer = (axis_y < 4); //위쪽이 컴퓨터
	this.kind = janggi_kind;
	this.score = janggi_kind.Score;

	this.text = '';
	this.obj_className = 'team_green ';
	this.team = Janggi.User;
	if(this.is_computer == true) {
		this.obj_className = 'team_red ';
		this.team = Janggi.Computer;
	}

	switch(janggi_kind) {
		case Janggi.Gung :
			this.text = '楚';
			if(this.is_computer == true) {
				this.text = '漢';
			}
			this.obj_className += 'item_big';
			break;
		case Janggi.Cha:
			this.text = '車';
			this.obj_className += 'item_normal';
			break;
		case Janggi.Sang:
			this.text = '象';
			this.obj_className += 'item_normal';
			break;
		case Janggi.Ma:
			this.text = '馬';
			this.obj_className += 'item_normal';
			break;
		case Janggi.Sa:
			this.text = '士';
			this.obj_className += 'item_small';
			break;
		case Janggi.Po:
			this.text = '包';
			this.obj_className += 'item_normal';
			break;
		case Janggi.Jole:
			this.text = '卒';
			if(this.is_computer == true) {
				this.text = '兵';
			}
			this.obj_className += 'item_small';
			break;
	}

	this.obj = document.createElement('div');
	this.obj.id = 'janggi_'+this.axis_x+'_'+this.axis_y;
	this.obj.className = this.obj_className;
	this.obj.innerHTML = this.text;
	this.obj.style.position = 'absolute';
	this.obj.style.left = this.px_x+'px';
	this.obj.style.top  = this.px_y+'px';

	this.MoveMotion = function(to_x, to_y, motion, after_params) {
		var mpx = this.px_x;
		var mpy = this.px_y;

		var dpx = to_x * this.x_term;
		var dpy = to_y * this.y_term;
		var parentObjName = motion.objName.split('.')[0];
		eval("var parentObj = "+parentObjName+";");
		var finish_script =  parentObjName+ '.ChangeUnit('+parseInt(after_params[0])+','+parseInt(after_params[1])+','+parseInt(after_params[2])+','+parseInt(after_params[3])+',true);';

		eval("var stage = "+parentObjName+".getStageStatus();");
		var eat_item = stage[after_params[2]][after_params[3]];
		if(eat_item != null && eat_item.junit_obj.team != this.team) {
			var eatitem_dest_y = (this.team == Janggi.User) ? parseInt(parentObj.height) + parseInt(this.y_term) : this.y_term*-1;

			finish_script = parentObjName+".motion.MoveTo(document.getElementById('"+eat_item.junit_obj.obj.id+"'), "+dpx+", "+eatitem_dest_y+", null, null, "+dpx+", "+dpy+", null, null, '"+finish_script+"', false);";
		}
		motion.MoveTo(this.obj, dpx, dpy, null, null, mpx, mpy, null, null, finish_script, false);
	}
	this.Move = function(to_x, to_y) {
		this.axis_x = to_x;
		this.axis_y = to_y;
		this.px_x = this.axis_x * this.x_term;
		this.px_y = this.axis_y * this.y_term;
		this.obj.style.left = this.px_x+'px';
		this.obj.style.top  = this.px_y+'px';
	}
	this.getAddScore = function(stage) {
		var result = 0;
		
		var poses = this.getMoveablePos(stage);

		for(var i = 0 ; i < poses.length ; i++) {
			if(stage[poses[i].x][poses[i].y] != null && stage[poses[i].x][poses[i].y].junit_obj.team != this.team) {
				result += stage[poses[i].x][poses[i].y].junit_obj.score /100;
			}
		}
		return result;
	}
	this.getMoveablePos = function(stage_state) {
		var now_x = this.axis_x;
		var now_y = this.axis_y;
		for(var col = 0 ; col <= 8 ; col++) {
			for (var row = 0 ; row <= 9 ; row++) {
				if(stage_state[col][row] == this) {
					now_x = col;
					now_y = row;
					col = 8;
					break;
				}
			}
		}

		var result = [];
		switch(this.kind) {
			case Janggi.Cha:
				//left
				for(var i = now_x-1 ; i >= 0 ; i--) {
					if(stage_state[i][now_y] == null) {
						result.push(new Point(i, now_y));
						continue;
					} else {
						if(stage_state[i][now_y].junit_obj.team != this.team) {
							result.push(new Point(i, now_y));
						}
						break;
					}
				}

				//right
				for(var i = now_x+1 ; i <= 8 ; i++) {
					if(stage_state[i][now_y] == null) {
						result.push(new Point(i, now_y));
						continue;
					} else {
						if(stage_state[i][now_y].junit_obj.team != this.team) {
							result.push(new Point(i, now_y));
						}
						break;
					}
				}

				//up
				for(var i = now_y-1 ; i >= 0 ; i--) {
					if(stage_state[now_x][i] == null) {
						result.push(new Point(now_x, i));
						continue;
					} else {
						if(stage_state[now_x][i].junit_obj.team != this.team) {
							result.push(new Point(now_x, i));
						}
						break;
					}
				}

				// down
				for(var i = now_y+1 ; i <= 9 ; i++) {
					if(stage_state[now_x][i] == null) {
						result.push(new Point(now_x, i));
						continue;
					} else {
						if(stage_state[now_x][i].junit_obj.team != this.team) {
							result.push(new Point(now_x, i));
						}
						break;
					}
				}

				// 대각
				if(now_y < 3) { //위쪽. 컴퓨터

					if(now_x-now_y == 3) { // 우로 대각선 이동
						switch(now_y) {
							case 0 :
								if(stage_state[now_x+1][now_y+1] == null || stage_state[now_x+1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y+1)); 
								}
								if(stage_state[now_x+1][now_y+1] == null && (stage_state[now_x+2][now_y+2] == null || stage_state[now_x+2][now_y+2].junit_obj.team != this.team)) {
									result.push(new Point(now_x+2, now_y+2)); 
								}
								break;
							case 1 :
								if(stage_state[now_x+1][now_y+1] == null || stage_state[now_x+1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y+1));
								}
								if(stage_state[now_x-1][now_y-1] == null || stage_state[now_x-1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y-1)); 
								}
								break;
							case 2 :
								if(stage_state[now_x-1][now_y-1] == null || stage_state[now_x-1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y-1)); 
								}
								if(stage_state[now_x-1][now_y-1] == null && (stage_state[now_x-2][now_y-2] == null || stage_state[now_x-2][now_y-2].junit_obj.team != this.team)) {
									result.push(new Point(now_x-2, now_y-2)); 
								}
								break;
						}
					} 
					if(now_x+now_y == 5) {
						switch(now_y) {
							case 0 :
								if(stage_state[now_x-1][now_y+1] == null || stage_state[now_x-1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y+1)); 
								}
								if(stage_state[now_x-1][now_y+1] == null && (stage_state[now_x-2][now_y+2] == null || stage_state[now_x-2][now_y+2].junit_obj.team != this.team)) {
									result.push(new Point(now_x-2, now_y+2)); 
								}
								break;
							case 1 :
								if(stage_state[now_x-1][now_y+1] == null || stage_state[now_x-1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y+1)); 
								}
								if(stage_state[now_x+1][now_y-1] == null || stage_state[now_x+1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y-1)); 
								}
								break;
							case 2 :
								if(stage_state[now_x+1][now_y-1] == null || stage_state[now_x+1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y-1)); 
								}
								if(stage_state[now_x+1][now_y-1] == null && (stage_state[now_x+2][now_y-2] == null || stage_state[now_x+2][now_y-2].junit_obj.team != this.team)) {
									result.push(new Point(now_x+2, now_y-2)); 
								}
								break;
						}
					}
				} else if(now_y > 6){ //아래 팀. 유저

					if(now_y - now_x == 4) { // 우로 대각선 이동
						switch(now_y) {
							case 7 :
								if(stage_state[now_x+1][now_y+1] == null || stage_state[now_x+1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y+1)); 
								}
								if(stage_state[now_x+1][now_y+1] == null && (stage_state[now_x+2][now_y+2] == null || stage_state[now_x+2][now_y+2].junit_obj.team != this.team)) {
									result.push(new Point(now_x+2, now_y+2)); 
								}
								break;
							case 8 :
								if(stage_state[now_x+1][now_y+1] == null || stage_state[now_x+1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y+1)); 
								}
								if(stage_state[now_x-1][now_y-1] == null || stage_state[now_x-1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y-1)); 
								}
								break;
							case 9 :
								if(stage_state[now_x-1][now_y-1] == null || stage_state[now_x-1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y-1)); 
								}
								if(stage_state[now_x-1][now_y-1] == null && (stage_state[now_x-2][now_y-2] == null || stage_state[now_x-2][now_y-2].junit_obj.team != this.team)) {
									result.push(new Point(now_x-2, now_y-2)); 
								}
								break;
						}
					} 
					if(now_x+now_y == 12) {
						switch(now_y) {
							case 7 :
								if(stage_state[now_x-1][now_y+1] == null || stage_state[now_x-1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y+1)); 
								}
								if(stage_state[now_x-1][now_y+1] == null && (stage_state[now_x-2][now_y+2] == null || stage_state[now_x-2][now_y+2].junit_obj.team != this.team)) {
									result.push(new Point(now_x-2, now_y+2)); 
								}
								break;
							case 8 :
								if(stage_state[now_x-1][now_y+1] == null || stage_state[now_x-1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y+1)); 
								}
								if(stage_state[now_x+1][now_y-1] == null || stage_state[now_x+1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y-1)); 
								}
								break;
							case 9 :
								if(stage_state[now_x+1][now_y-1] == null || stage_state[now_x+1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y-1)); 
								}
								if(stage_state[now_x+1][now_y-1] == null && (stage_state[now_x+2][now_y-2] == null || stage_state[now_x+2][now_y-2].junit_obj.team != this.team)) {
									result.push(new Point(now_x+2, now_y-2)); 
								}
								break;
						}
					}
				}
				return result;
				break;
			case Janggi.Gung :
			case Janggi.Sa:

				var temp_result = [];

				if(now_x > 3) { // left
					temp_result.push(new Point(now_x-1, now_y));
				}
				if(now_x < 5) { // left
					temp_result.push(new Point(now_x+1, now_y));
				}


				if(now_y < 3) { //위쪽. 컴퓨터
					if(now_y < 2) {
						temp_result.push(new Point(now_x, now_y+1)); //down
					}
					if(now_y > 0) {
						temp_result.push(new Point(now_x, now_y-1)); //up
					}

					if(now_x-now_y == 3) { // 우로 대각선 이동
						switch(now_y) {
							case 0 :
								temp_result.push(new Point(now_x+1, now_y+1)); 
								break;
							case 1 :
								temp_result.push(new Point(now_x+1, now_y+1));
								temp_result.push(new Point(now_x-1, now_y-1)); 
								break;
							case 2 :
								temp_result.push(new Point(now_x-1, now_y-1)); 
								break;
						}
					} 
					if(now_x+now_y == 5) {
						switch(now_y) {
							case 0 :
								temp_result.push(new Point(now_x-1, now_y+1)); 
								break;
							case 1 :
								temp_result.push(new Point(now_x-1, now_y+1)); 
								temp_result.push(new Point(now_x+1, now_y-1)); 
								break;
							case 2 :
								temp_result.push(new Point(now_x+1, now_y-1)); 
								break;
						}
					}
				} else if(now_y > 6){ //아래 팀. 유저
					if(now_y < 9) {
						temp_result.push(new Point(now_x, now_y+1)); //down
					}
					if(now_y > 7) {
						temp_result.push(new Point(now_x, now_y-1)); //up
					}

					if(now_y - now_x == 4) { // 우로 대각선 이동
						switch(now_y) {
							case 7 :
								temp_result.push(new Point(now_x+1, now_y+1)); 
								break;
							case 8 :
								temp_result.push(new Point(now_x+1, now_y+1)); 
								temp_result.push(new Point(now_x-1, now_y-1)); 
								break;
							case 9 :
								temp_result.push(new Point(now_x-1, now_y-1)); 
								break;
						}
					} 
					if(now_x+now_y == 12) {
						switch(now_y) {
							case 7 :
								temp_result.push(new Point(now_x-1, now_y+1)); 
								break;
							case 8 :
								temp_result.push(new Point(now_x-1, now_y+1)); 
								temp_result.push(new Point(now_x+1, now_y-1)); 
								break;
							case 9 :
								temp_result.push(new Point(now_x+1, now_y-1)); 
								break;
						}
					}
				}

				for(var i = 0 ; i < temp_result.length ; i++) {
					if(stage_state[temp_result[i].x][temp_result[i].y] == null || stage_state[temp_result[i].x][temp_result[i].y].junit_obj.team != this.team) {
						result.push(temp_result[i]);
					}
				}
				return result;
				break;
			
			case Janggi.Sang:
				if(now_y > 2) { // up
					if(stage_state[now_x][now_y-1] == null) {
						if(now_x > 1 && stage_state[now_x-1][now_y-2] == null && (stage_state[now_x-2][now_y-3] == null || stage_state[now_x-2][now_y-3].junit_obj.team != this.team )) {
							result.push(new Point(now_x-2, now_y-3)); // up left
						}
						if(now_x < 7 && stage_state[now_x+1][now_y-2] == null && (stage_state[now_x+2][now_y-3] == null || stage_state[now_x+2][now_y-3].junit_obj.team != this.team )) {
							result.push(new Point(now_x+2, now_y-3)); // up right
						}
					}
				}
				if(now_y < 7) { // down 
					if(stage_state[now_x][now_y+1] == null) {
						if(now_x > 1 && stage_state[now_x-1][now_y+2] == null && (stage_state[now_x-2][now_y+3] == null || stage_state[now_x-2][now_y+3].junit_obj.team != this.team )) {
							result.push(new Point(now_x-2, now_y+3)); // down left
						}
						if(now_x < 7 && stage_state[now_x+1][now_y+2] == null && (stage_state[now_x+2][now_y+3] == null || stage_state[now_x+2][now_y+3].junit_obj.team != this.team )) {
							result.push(new Point(now_x+2, now_y+3)); // down right
						}
					}
				}

				if(now_x > 2) { //left
					if(stage_state[now_x-1][now_y] == null) {
						if(now_y > 1 && stage_state[now_x-2][now_y-1] == null && (stage_state[now_x-3][now_y-2] == null || stage_state[now_x-3][now_y-2].junit_obj.team != this.team )) {
							result.push(new Point(now_x-3, now_y-2)); // left up
						}
						if(now_y < 8 && stage_state[now_x-2][now_y+1] == null && (stage_state[now_x-3][now_y+2] == null || stage_state[now_x-3][now_y+2].junit_obj.team != this.team )) {
							result.push(new Point(now_x-3, now_y+2)); // left down
						}
					}
				}
				if(now_x < 6) { // right
					if(stage_state[now_x+1][now_y] == null) {
						if(now_y > 0 && stage_state[now_x+2][now_y-1] == null && (stage_state[now_x+3][now_y-2] == null || stage_state[now_x+3][now_y-2].junit_obj.team != this.team )) {
							result.push(new Point(now_x+3, now_y-2)); // right up
						}
						if(now_y < 9 && stage_state[now_x+2][now_y+1] == null && (stage_state[now_x+3][now_y+2] == null || stage_state[now_x+3][now_y+2].junit_obj.team != this.team )) {
							result.push(new Point(now_x+3, now_y+2)); // right down
						}
					}
				}
				
				return result;
				break;
			case Janggi.Ma:
				if(now_y > 1) { // up
					if(stage_state[now_x][now_y-1] == null) {
						if(now_x > 0 && (stage_state[now_x-1][now_y-2] == null || stage_state[now_x-1][now_y-2].junit_obj.team != this.team )) {
							result.push(new Point(now_x-1, now_y-2)); // up left
						}
						if(now_x < 8 && (stage_state[now_x+1][now_y-2] == null || stage_state[now_x+1][now_y-2].junit_obj.team != this.team )) {
							result.push(new Point(now_x+1, now_y-2)); // up right
						}
					}
				}
				if(now_y < 8) { // down 
					if(stage_state[now_x][now_y+1] == null) {
						if(now_x > 0 && (stage_state[now_x-1][now_y+2] == null || stage_state[now_x-1][now_y+2].junit_obj.team != this.team )) {
							result.push(new Point(now_x-1, now_y+2)); // down left
						}
						if(now_x < 8 && (stage_state[now_x+1][now_y+2] == null || stage_state[now_x+1][now_y+2].junit_obj.team != this.team )) {
							result.push(new Point(now_x+1, now_y+2)); // down right
						}
					}
				}

				if(now_x > 1) { //left
					if(stage_state[now_x-1][now_y] == null) {
						if(now_y > 0 && (stage_state[now_x-2][now_y-1] == null || stage_state[now_x-2][now_y-1].junit_obj.team != this.team )) {
							result.push(new Point(now_x-2, now_y-1)); // left up
						}
						if(now_y < 9 && (stage_state[now_x-2][now_y+1] == null || stage_state[now_x-2][now_y+1].junit_obj.team != this.team )) {
							result.push(new Point(now_x-2, now_y+1)); // left down
						}
					}
				}
				if(now_x < 7) { // right
					if(stage_state[now_x+1][now_y] == null) {
						if(now_y > 0 && (stage_state[now_x+2][now_y-1] == null || stage_state[now_x+2][now_y-1].junit_obj.team != this.team )) {
							result.push(new Point(now_x+2, now_y-1)); // right up
						}
						if(now_y < 9 && (stage_state[now_x+2][now_y+1] == null || stage_state[now_x+2][now_y+1].junit_obj.team != this.team )) {
							result.push(new Point(now_x+2, now_y+1)); // right down
						}
					}
				}
				
				return result;
				break;
			case Janggi.Po:
				// left
				if(now_x > 1) {
					var jump_flag = false;
					for(var i = now_x-1 ; i >= 0 ; i--) {
						if(jump_flag == false) { 
							// 뛰어넘기 전
							if(stage_state[i][now_y] != null) { // 넘을 장기알 발견
								if(stage_state[i][now_y].junit_obj.kind == Janggi.Po) {// 포는 넘지 못한다.
									break;
								}
								jump_flag = true;
							}
							continue;
						}

						// 뛰어넘은 후
						if(stage_state[i][now_y] == null) { // 비었거나 
							result.push(new Point(i, now_y));
						} else {
							if (stage_state[i][now_y].junit_obj.kind != Janggi.Po && stage_state[i][now_y].junit_obj.team  != this.team) //포가 아닌 적의 장기알까지 갈 수 있다.
								result.push(new Point(i, now_y));
							break;
						}
					}
				}


				// right
				if(now_x < 7) {
					var jump_flag = false;
					for(var i = now_x+1 ; i <= 8 ; i++) {
						if(jump_flag == false) { 
							// 뛰어넘기 전
							if(stage_state[i][now_y] != null) { // 넘을 장기알 발견
								if(stage_state[i][now_y].junit_obj.kind == Janggi.Po) {// 포는 넘지 못한다.
									break;
								}
								jump_flag = true;
							}
							continue;
						}

						// 뛰어넘은 후
						if(stage_state[i][now_y] == null) { // 비었거나 
							result.push(new Point(i, now_y));
						} else {
							if (stage_state[i][now_y].junit_obj.kind != Janggi.Po && stage_state[i][now_y].junit_obj.team  != this.team) //적의 장기알까지 갈 수 있다.
								result.push(new Point(i, now_y));
							break;
						}
					}
				}

				// up
				if(now_y > 1) {
					var jump_flag = false;
					for(var i = now_y-1 ; i >= 0 ; i--) {
						if(jump_flag == false) { 
							// 뛰어넘기 전
							if(stage_state[now_x][i] != null) { // 넘을 장기알 발견
								if(stage_state[now_x][i].junit_obj.kind == Janggi.Po) {// 포는 넘지 못한다.
									break;
								}
								jump_flag = true;
							}
							continue;
						}

						// 뛰어넘은 후
						if(stage_state[now_x][i] == null) { // 비었거나 
							result.push(new Point(now_x, i));
						} else {
							if (stage_state[now_x][i].junit_obj.kind != Janggi.Po && stage_state[now_x][i].junit_obj.team  != this.team) //적의 장기알까지 갈 수 있다.
								result.push(new Point(now_x, i));
							break;
						}
					}
				}

				// down 
				if(now_y < 8) {
					var jump_flag = false;
					for(var i = now_y+1 ; i <= 9 ; i++) {
						if(jump_flag == false) { 
							// 뛰어넘기 전
							if(stage_state[now_x][i] != null) { // 넘을 장기알 발견
								if(stage_state[now_x][i].junit_obj.kind == Janggi.Po) {// 포는 넘지 못한다.
									break;
								}
								jump_flag = true;
							}
							continue;
						}

						// 뛰어넘은 후
						if(stage_state[now_x][i] == null) { // 비었거나 
							result.push(new Point(now_x, i));
						} else {
							if (stage_state[now_x][i].junit_obj.kind != Janggi.Po && stage_state[now_x][i].junit_obj.team  != this.team) //적의 장기알까지 갈 수 있다.
								result.push(new Point(now_x, i));
							break;
						}
					}
				}

				// 대각
				if(now_y < 3) { //위쪽. 컴퓨터

					if(now_x-now_y == 3) { // 우로 대각선 이동
						switch(now_y) {
							case 0 :
								if(stage_state[now_x+1][now_y+1] != null && (stage_state[now_x+2][now_y+2] == null || stage_state[now_x+2][now_y+2].junit_obj.team != this.team)) {
									result.push(new Point(now_x+2, now_y+2)); 
								}
								break;
							case 1 :
								break;
							case 2 :
								if(stage_state[now_x-1][now_y-1] != null && (stage_state[now_x-2][now_y-2] == null || stage_state[now_x-2][now_y-2].junit_obj.team != this.team)) {
									result.push(new Point(now_x-2, now_y-2)); 
								}
								break;
						}
					} 
					if(now_x+now_y == 5) {
						switch(now_y) {
							case 0 :
								if(stage_state[now_x-1][now_y+1] != null && (stage_state[now_x-2][now_y+2] == null || stage_state[now_x-2][now_y+2].junit_obj.team != this.team)) {
									result.push(new Point(now_x-2, now_y+2)); 
								}
								break;
							case 1 :
								break;
							case 2 :
								if(stage_state[now_x+1][now_y-1] != null && (stage_state[now_x+2][now_y-2] == null || stage_state[now_x+2][now_y-2].junit_obj.team != this.team)) {
									result.push(new Point(now_x+2, now_y-2)); 
								}
								break;
						}
					}
				} else if(now_y > 6){ //아래 팀. 유저

					if(now_y - now_x == 4) { // 우로 대각선 이동
						switch(now_y) {
							case 7 :
								if(stage_state[now_x+1][now_y+1] != null && (stage_state[now_x+2][now_y+2] == null || stage_state[now_x+2][now_y+2].junit_obj.team != this.team)) {
									result.push(new Point(now_x+2, now_y+2)); 
								}
								break;
							case 8 :
								break;
							case 9 :
								if(stage_state[now_x-1][now_y-1] != null && (stage_state[now_x-2][now_y-2] == null || stage_state[now_x-2][now_y-2].junit_obj.team != this.team)) {
									result.push(new Point(now_x-2, now_y-2)); 
								}
								break;
						}
					} 
					if(now_x+now_y == 12) {
						switch(now_y) {
							case 7 :
								if(stage_state[now_x-1][now_y+1] != null && (stage_state[now_x-2][now_y+2] == null || stage_state[now_x-2][now_y+2].junit_obj.team != this.team)) {
									result.push(new Point(now_x-2, now_y+2)); 
								}
								break;
							case 8 :
								break;
							case 9 :
								if(stage_state[now_x+1][now_y-1] != null && (stage_state[now_x+2][now_y-2] == null || stage_state[now_x+2][now_y-2].junit_obj.team != this.team)) {
									result.push(new Point(now_x+2, now_y-2)); 
								}
								break;
						}
					}
				}
				return result;
				break;
			case Janggi.Jole:
				// left
				if(now_x > 0 && (stage_state[now_x-1][now_y] == null || stage_state[now_x-1][now_y].junit_obj.is_computer != this.is_computer)){
					result.push(new Point(now_x-1, now_y));
				}
				// right
				if(now_x < 8 && (stage_state[now_x+1][now_y] == null || stage_state[now_x+1][now_y].junit_obj.is_computer != this.is_computer)){
					result.push(new Point(now_x+1, now_y));
				}

				// computer : down, user:up
				if(this.is_computer) {
					if(now_y < 9 && (stage_state[now_x][now_y+1] == null || stage_state[now_x][now_y+1].junit_obj.is_computer != this.is_computer)){
						result.push(new Point(now_x, now_y+1));
					}
				} else {
					if(now_y > 0 && (stage_state[now_x][now_y-1] == null || stage_state[now_x][now_y-1].junit_obj.is_computer != this.is_computer)){
						result.push(new Point(now_x, now_y-1));
					}
				}

				// 대각
				if(now_y < 3) { //위쪽. 컴퓨터

					if(now_x-now_y == 3) { // 우로 대각선 이동
						switch(now_y) {
							case 0 :
								break;
							case 1 :
								if(stage_state[now_x-1][now_y-1] == null || stage_state[now_x-1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y-1)); 
								}
								break;
							case 2 :
								if(stage_state[now_x-1][now_y-1] == null || stage_state[now_x-1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y-1)); 
								}
								break;
						}
					} 
					if(now_x+now_y == 5) {
						switch(now_y) {
							case 0 :
								break;
							case 1 :
								if(stage_state[now_x+1][now_y-1] == null || stage_state[now_x+1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y-1)); 
								}
								break;
							case 2 :
								if(stage_state[now_x+1][now_y-1] == null || stage_state[now_x+1][now_y-1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y-1)); 
								}
								break;
						}
					}
				} else if(now_y > 6){ //아래 팀. 유저

					if(now_y - now_x == 4) { // 우로 대각선 이동
						switch(now_y) {
							case 7 :
								if(stage_state[now_x+1][now_y+1] == null || stage_state[now_x+1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y+1)); 
								}
								break;
							case 8 :
								if(stage_state[now_x+1][now_y+1] == null || stage_state[now_x+1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x+1, now_y+1)); 
								}
								break;
							case 9 :
								break;
						}
					} 
					if(now_x+now_y == 12) {
						switch(now_y) {
							case 7 :
								if(stage_state[now_x-1][now_y+1] == null || stage_state[now_x-1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y+1)); 
								}
								break;
							case 8 :
								if(stage_state[now_x-1][now_y+1] == null || stage_state[now_x-1][now_y+1].junit_obj.team != this.team) {
									result.push(new Point(now_x-1, now_y+1)); 
								}
								break;
							case 9 :
								break;
						}
					}
				}

				return result;
				break;
		}
	}
	
}

// 장기알 정보
var Units = function(obj, is_valid) {
	this.junit_obj = obj;
	this.is_valid = is_valid;
}


