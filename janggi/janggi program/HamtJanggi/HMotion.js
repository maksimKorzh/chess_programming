/**********************************************************
/*      엘리먼트 이동 및 사이즈 변경, 변경간 모션 효과 주는 모듈
/*      by 함승목   2010. 8. 26
/*
**********************************************************/
var HMotion = function(objName)
{
	this.objName = objName;
	this.workobj;
	this.workflag = false;
	this.destx;
	this.desty;

	this.before_zindex = '';

	this.objlist = [];

	this.prevsetx;
	this.prevsety;

	this.count=0;

	var frame = 14;

	var firstflag = true;
	var movepointsX = [];
	var movepointsY = [];
	var movepointsW = [];
	var movepointsH = [];


	this.MoveTo = function(obj, destx, desty, destw, desth, myx, myy, myw, myh, pendfunc, is_percent)
	{

		if(this.workflag && this.workobj != obj)
			return;

		workflag = true;
		workobj = obj;

		if(firstflag)
		{
			firstflag = false;
			this.count = 0;

			this.before_zindex = obj.style.zIndex;
			obj.style.zIndex = 10000;

			movepointsX = [];
			movepointsY = [];
			movepointsW = [];
			movepointsH = [];

			if(myx == null)
				myx =  this.getElementAbsPosX(obj);
			if(myy == null)
				myy =  this.getElementAbsPosY(obj);
			if(myw == null)
				myw =  obj.offsetWidth;
			if(myh == null)
				myw =  obj.offsetWidth;

			if(destx == null)
				destx =  this.getElementAbsPosX(obj);
			if(desty == null)
				desty =  this.getElementAbsPosY(obj);
			if(destw == null)
				destw =  obj.offsetWidth;
			if(desth == null)
				desth = obj.offsetHeight;


			var log = 'myw : '+myw+', destw : '+destw+"\n";
			var distx = destx-myx;
			var disty = desty-myy;
			var distw = destw-myw;
			var disth = desth-myh;

			log += 'myw : '+myw+', destw : '+destw+"\n";
			for(var i = 1 ; i <= frame ; i++)
			{
				movepointsX[i-1] = myx+Math.floor(distx * Math.sin(Math.PI/2*i/frame));
				movepointsY[i-1] = myy+Math.floor(disty * Math.sin(Math.PI/2*i/frame));
				movepointsW[i-1] = myw+Math.floor(distw * Math.sin(Math.PI/2*i/frame));
				movepointsH[i-1] = myh+Math.floor(disth * Math.sin(Math.PI/2*i/frame));

				log += ', '+movepointsW[i-1];
			}
		}

		is_percent = (is_percent == null || !is_percent || is_percent == 'px') ? 'px' : '%';
		if(this.count < movepointsX.length)
		{
			obj.style.left = movepointsX[this.count]+'px';
			obj.style.top = movepointsY[this.count]+'px';
			obj.style.width = movepointsW[this.count]+is_percent;
			obj.style.height = movepointsH[this.count]+'px';

			this.count++;



			setTimeout(this.objName+'.MoveTo(document.getElementById("'+obj.id+'"), '+destx+', '+desty + ', '+destw+', '+desth+', '+myx+', '+myy+', '+myw+', '+myh+', "'+pendfunc+'", "'+is_percent+'")', 20);
			return;
		}
		else
		{
			firstflag = true;

			obj.style.zIndex = this.before_zindex;
			if(pendfunc)
				eval(pendfunc);

			this.workflag = false;
			return;
		}

	}




	this.SetTo = function(obj, x, y)
	{
		obj.style.position = 'absolute';
		obj.style.top = y+'px';
		obj.style.left = x+'px';
	}
	//////////////////////////////////////////

	this.SetAbsolute = function(obj)
	{
		var x = this.getElementAbsPosX(obj);
		var y = this.getElementAbsPosY(obj);

		obj.style.position = 'absolute';
		obj.style.top = y+'px';
		obj.style.left = x+'px';
	}
	this.SetRelative = function(obj)
	{
		obj.style.position = 'relative';
	}
	this.getElementAbsPosX = function(el)
	{
		var dx = 0;
		if (el.offsetParent) {
			dx = el.offsetLeft;
			while (el = el.offsetParent) {
				dx += el.offsetLeft;
			}
		}
		return dx;
	}

	this.getElementAbsPosY = function (el)
	{
		var dy = 0;
		if (el.offsetParent) {
			dy = el.offsetTop ;
			while (el = el.offsetParent) {
				dy += el.offsetTop;
			}
		}
		return dy;
	}
	this.myTest = function(str)
	{
		document.getElementById('test').innerHTML = str;
	}


}


