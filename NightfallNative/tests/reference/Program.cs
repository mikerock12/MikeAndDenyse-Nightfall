using System;
using System.Globalization;
using Nightfall;
class Program {
    static Ctl Input(int frame) {int cycle=frame%120;return new Ctl {R=cycle<90,L=cycle>=90&&cycle<110,D=cycle>=65&&cycle<77,Jp=cycle==15||cycle==50,Jn=(cycle>=15&&cycle<29)||(cycle>=50&&cycle<61),Ap=frame%40==5};}
    static void Main() {
        CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;
        for(int world=0;world<16;++world) {
            var level=LevelBuilder.Compile(world);uint hash=2166136261;
            foreach(var row in level.Tiles)foreach(byte tile in row){hash^=tile;hash=unchecked(hash*16777619);}
            Console.WriteLine($"LEVEL,{world},{level.Cols},{level.Rows},{level.Spawn.x:R},{level.Spawn.y:R},{level.ArenaX0:R},{level.Exit.x:R},{hash}");
            foreach(string hero in new[]{"mike","denyse"})foreach(int fps in new[]{60,30,20}) {
                var sim=new GameSim();Ctl ctl=new Ctl();sim.ReadCtl=slot=>ctl;sim.Start(world,hero,hero=="mike"?"denyse":"mike",false);
                sim.Enemies.Clear();sim.Bosses.Clear();sim.Items.Clear();sim.Players[0].Lives=999;
                for(int frame=0;frame<720;++frame){ctl=Input(frame);sim.Tick(1f/fps);sim.Fx.Clear();var p=sim.Players[0];Console.WriteLine($"STEP,{world},{hero},{fps},{frame},{p.X:R},{p.Y:R},{p.Vx:R},{p.Vy:R},{p.Hp},{p.Lives},{(p.OnGround?1:0)},{p.Atk:R},{p.SafeX:R},{p.SafeY:R},{p.Anim}");}
            }
        }
        // Compare original AI and damage with the same explicitly injected random stream.
        // Isolate each enemy so no changes in unrelated spawn ordering can hide a rule mismatch.
        foreach(string hero in new[]{"mike","denyse"})foreach(var enemy in Catalog.Enemies) {
            UnityEngine.Random.InitState(1234);
            var sim=new GameSim();Ctl ctl=new Ctl();sim.ReadCtl=slot=>ctl;sim.Start(0,hero,"denyse",false);
            sim.Enemies.Clear();sim.Bosses.Clear();sim.Items.Clear();sim.Players[0].Lives=999;
            var player=sim.Players[0];player.Inv=99;
            var actor=new EnemyA {Id=enemy.Id,Def=enemy,Kind=Catalog.EnemyIndex(enemy.Id),Hp=enemy.Hp,MaxHp=enemy.Hp,W=enemy.W,H=enemy.H,X=player.X+55,Y=player.Y+player.H-enemy.H,Dir=-1,Facing=-1};
            actor.HomeX=actor.X;actor.HomeY=actor.Y;sim.Enemies.Add(actor);
            for(int frame=0;frame<240;++frame){ctl=new Ctl {Ap=frame%24==0,Jp=frame==120,Jn=frame>=120&&frame<135,R=frame>150&&frame<180};sim.Tick(1f/60);sim.Fx.Clear();Console.WriteLine($"COMBAT,{hero},{enemy.Id},{frame},{player.X:R},{player.Y:R},{player.Hp},{actor.X:R},{actor.Y:R},{actor.Hp},{(actor.Dead?1:0)},{sim.Projectiles.Count},{sim.Score},{sim.Souls}");}
        }
        var bosses=new System.Collections.Generic.List<BossDef>(Catalog.Bosses);bosses.Add(Catalog.Belial);
        foreach(string hero in new[]{"mike","denyse"})foreach(var boss in bosses) {
            UnityEngine.Random.InitState(5678);var sim=new GameSim();Ctl ctl=new Ctl();sim.ReadCtl=slot=>ctl;sim.Start(boss.Id=="warden"||boss.Id=="belial"?15:0,hero,"denyse",false);
            sim.Enemies.Clear();sim.Bosses.Clear();sim.Items.Clear();var player=sim.Players[0];player.Lives=999;player.Inv=99;
            player.X=sim.Level.ArenaX0+200;player.Y=sim.Level.BossAt.y-player.H;sim.LockedArena=true;
            sim.Bosses.Add(new BossA {Id=boss.Id,Def=boss,Hp=boss.Hp,MaxHp=boss.Hp,W=boss.W,H=boss.H,X=player.X+60,Y=player.Y+player.H-boss.H,Facing=-1,Dir=-1,Intro=0});
            for(int frame=0;frame<480;++frame){ctl=new Ctl {Ap=frame%24==0,Jp=frame%120==60,Jn=frame%120>=60&&frame%120<75,R=frame%180>100&&frame%180<130,L=frame%180>=130&&frame%180<150};sim.Tick(1f/60);sim.Fx.Clear();var b=sim.Bosses[sim.Bosses.Count-1];Console.WriteLine($"BOSS,{hero},{boss.Id},{frame},{player.X:R},{player.Y:R},{player.Hp},{b.X:R},{b.Y:R},{b.Hp},{(b.Dead?1:0)},{sim.Projectiles.Count},{sim.Score},{sim.Souls},{b.Id},{b.State},{sim.State}");}
        }
    }
}
