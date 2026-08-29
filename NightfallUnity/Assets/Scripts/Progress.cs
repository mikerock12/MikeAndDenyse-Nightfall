using UnityEngine;

namespace Nightfall
{
    public static class Progress
    {
        const string KeyUnlock = "nightfall.unlocked";
        const string KeyHero = "nightfall.hero";
        const string KeySouls = "nightfall.souls";
        const string KeyScore = "nightfall.score";
        const string KeyLast = "nightfall.last";

        public const int WorldCount = 16;

        public static int Unlocked = 1;
        public static string Hero = "mike";
        public static int Souls;
        public static int Score;
        public static int LastWorld;

        public static void Load()
        {
            Unlocked = Mathf.Clamp(PlayerPrefs.GetInt(KeyUnlock, 1), 1, WorldCount);
            Hero = PlayerPrefs.GetString(KeyHero, "mike");
            if (Hero != "mike" && Hero != "denyse") Hero = "mike";
            Souls = Mathf.Max(0, PlayerPrefs.GetInt(KeySouls, 0));
            Score = Mathf.Max(0, PlayerPrefs.GetInt(KeyScore, 0));
            LastWorld = Mathf.Clamp(PlayerPrefs.GetInt(KeyLast, 0), 0, Unlocked - 1);
        }

        public static void Save()
        {
            PlayerPrefs.SetInt(KeyUnlock, Unlocked);
            PlayerPrefs.SetString(KeyHero, Hero ?? "mike");
            PlayerPrefs.SetInt(KeySouls, Souls);
            PlayerPrefs.SetInt(KeyScore, Score);
            PlayerPrefs.SetInt(KeyLast, LastWorld);
            PlayerPrefs.Save();
        }

        public static bool CanPlay(int worldIndex)
        {
            return worldIndex >= 0 && worldIndex < Unlocked && worldIndex < WorldCount;
        }

        public static void OnClear(int worldIndex)
        {
            if (worldIndex < 0) return;
            Unlocked = Mathf.Max(Unlocked, Mathf.Min(WorldCount, worldIndex + 2));
            LastWorld = Mathf.Clamp(worldIndex + 1, 0, WorldCount - 1);
            Save();
        }

        public static void RememberRun(int souls, int score, string hero)
        {
            Souls = souls;
            Score = score;
            if (hero == "mike" || hero == "denyse") Hero = hero;
            Save();
        }

        public static void ResetAll()
        {
            Unlocked = 1;
            Hero = "mike";
            Souls = 0;
            Score = 0;
            LastWorld = 0;
            Save();
        }
    }
}
