#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "DatabaseEnv.h"
#include "QueryResult.h"
#include "Config.h"
#include "WorldSession.h"


// Variables to detrmine if we auto sell for real players and bots
static bool IgnoreRealPlayers = true;
static std::string BotAccountPrefix = "rndbot";

// Loads Config Values
static void LoadJunkToGoldConfig()
{
    IgnoreRealPlayers = sConfigMgr->GetOption<bool>("JunkToGold.IgnoreRealPlayers", true);
    BotAccountPrefix = sConfigMgr->GetOption<std::string>("JunkToGold.BotAccountPrefix", "rndbot");
}

class JunkToGoldWorldScript : public WorldScript
{
public:
    JunkToGoldWorldScript() : WorldScript("JunkToGoldWorldScript") {}

    void OnStartup() override
    {
        LOG_INFO("server.loading", "[Junk2Gold] Loading Configuration");
        LoadJunkToGoldConfig();
    }
};

class JunkToGold : public PlayerScript
{   

public:
    JunkToGold() : PlayerScript("JunkToGold") {}       

    void OnPlayerLootItem(Player* player, Item* item, uint32 count, ObjectGuid /*lootguid*/) override
    {
        if (!item || !item->GetTemplate())
        {
            return;
        }

        // Make this apply only to bots
        if (IgnoreRealPlayers)
        {
            QueryResult result = LoginDatabase.Query("SELECT COUNT(username) FROM account WHERE id IN (SELECT account FROM acore_characters.characters WHERE NAME = '{}') AND username NOT LIKE '%{}%'", player->GetName(), BotAccountPrefix);
            uint32 resultCount = 0;
            resultCount = result->Fetch()->Get<uint32>();

            if (resultCount >= 1)
            {
                // This is a real player, dont sell their junk
                LOG_INFO("module", "[Junk2Gold] Player {} is a real person, not selling their junk", player->GetName());
                return;
            }
        }


        if (item->GetTemplate()->Quality == ITEM_QUALITY_POOR)
        {
            SendTransactionInformation(player, item, count);
            player->ModifyMoney(item->GetTemplate()->SellPrice * count);
            player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
        }
    }

private:
    void SendTransactionInformation(Player* player, Item* item, uint32 count)
    {
        std::string name;
        if (count > 1)
        {
            name = Acore::StringFormat("|cff9d9d9d|Hitem:{}::::::::80:::::|h[{}]|h|rx{}", item->GetTemplate()->ItemId, item->GetTemplate()->Name1, count);
        }
        else
        {
            name = Acore::StringFormat("|cff9d9d9d|Hitem:{}::::::::80:::::|h[{}]|h|r", item->GetTemplate()->ItemId, item->GetTemplate()->Name1);
        }

        uint32 money = item->GetTemplate()->SellPrice * count;
        uint32 gold = money / GOLD;
        uint32 silver = (money % GOLD) / SILVER;
        uint32 copper = (money % GOLD) % SILVER;

        std::string info;
        if (money < SILVER)
        {
            info = Acore::StringFormat("{} sold for {} copper.", name, copper);
        }
        else if (money < GOLD)
        {
            if (copper > 0)
            {
                info = Acore::StringFormat("{} sold for {} silver and {} copper.", name, silver, copper);
            }
            else
            {
                info = Acore::StringFormat("{} sold for {} silver.", name, silver);
            }
        }
        else
        {
            if (copper > 0 && silver > 0)
            {
                info = Acore::StringFormat("{} sold for {} gold, {} silver and {} copper.", name, gold, silver, copper);
            }
            else if (copper > 0)
            {
                info = Acore::StringFormat("{} sold for {} gold and {} copper.", name, gold, copper);
            }
            else if (silver > 0)
            {
                info = Acore::StringFormat("{} sold for {} gold and {} silver.", name, gold, silver);
            }
            else
            {
                info = Acore::StringFormat("{} sold for {} gold.", name, gold);
            }
        }

        ChatHandler(player->GetSession()).SendSysMessage(info);
    }
};

void Addmod_junk_to_goldScripts()
{
    new JunkToGold();
}
