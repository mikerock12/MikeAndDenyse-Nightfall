// Original gameplay sources compile unchanged. This is System.MathF execution,
// not Unity rendering, UnityEngine.Random equivalence, or device validation.
using System;
namespace UnityEngine {
    public struct Vector2 {public float x,y;public Vector2(float xx,float yy){x=xx;y=yy;}public float sqrMagnitude=>x*x+y*y;}
    public static class Mathf {
        public const float Rad2Deg=57.29578f;
        public static float Abs(float x)=>MathF.Abs(x);public static int Abs(int x)=>Math.Abs(x);
        public static float Floor(float x)=>MathF.Floor(x);public static int FloorToInt(float x)=>(int)MathF.Floor(x);
        public static int CeilToInt(float x)=>(int)MathF.Ceiling(x);public static int RoundToInt(float x)=>(int)MathF.Round(x,MidpointRounding.ToEven);
        public static float Sin(float x)=>MathF.Sin(x);public static float Atan2(float y,float x)=>MathF.Atan2(y,x);public static float Sqrt(float x)=>MathF.Sqrt(x);
        public static float Sign(float x)=>x>=0?1:-1;
        public static float Clamp(float x,float min,float max)=>Math.Clamp(x,min,max);public static int Clamp(int x,int min,int max)=>Math.Clamp(x,min,max);
        public static float Clamp01(float x)=>Math.Clamp(x,0,1);
        public static float Max(float a,float b)=>MathF.Max(a,b);public static int Max(int a,int b)=>Math.Max(a,b);
        public static float Min(float a,float b)=>MathF.Min(a,b);public static int Min(int a,int b)=>Math.Min(a,b);
        public static float Lerp(float a,float b,float t)=>a+(b-a)*Clamp01(t);
        public static float DeltaAngle(float current,float target){float d=(target-current)%360;if(d<0)d+=360;if(d>180)d-=360;return d;}
    }
    public static class Random {
        // Controlled input for differential tests; deliberately not presented as Unity's RNG.
        static uint state=1;
        public static void InitState(uint seed){state=seed==0?1:seed;}
        public static float value { get {uint x=state;x^=x<<13;x^=x>>17;x^=x<<5;state=x;return (x&0x00ffffffu)/16777216f;} }
        public static int Range(int min,int max)=>max<=min?min:min+Math.Min(max-min-1,(int)(value*(max-min)));
    }
}
namespace Nightfall {public static class ArtGen {public static float Hash(float x,float y,float seed){float v=MathF.Sin(x*127.1f+y*311.7f+seed*74.7f)*43758.5453f;return v-MathF.Floor(v);}}}
namespace Nightfall.Net {
    public struct EntSnap {public float X,Y;public byte Hp,Kind,Flags;}
    public struct ProjSnap {public float X,Y,Vx,Vy;public byte Kind,Flags;}
}
