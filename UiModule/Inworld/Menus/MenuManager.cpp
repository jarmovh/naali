//$ HEADER_MOD_FILE $ 
// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "MenuManager.h"
#include "Common/AnchorLayoutManager.h"
#include "UiProxyWidget.h"
#include "ActionNode.h"
#include "GroupNode.h"

#include <QSequentialAnimationGroup>
#include <QPropertyAnimation>
#include <QTimer>
#include <QUuid>
#include <QDebug>
#include <QGraphicsScene>

#include "MemoryLeakCheck.h"
#include "LoggingFunctions.h"

DEFINE_POCO_LOGGING_FUNCTIONS("MenuManager")

namespace CoreUi
{
    QString MenuManager::defaultItemIcon = "./data/ui/images/menus/edbutton_MATWIZ_normal.png";
    QString MenuManager::defaultGroupIcon = "./data/ui/images/menus/edbutton_WRLDTOOLS_icon.png";

    MenuManager::MenuManager(QObject *parent, CoreUi::AnchorLayoutManager *layout_manager) :
            QObject(parent),
            layout_manager_(layout_manager),
            last_clicked_node_(0),
            next_move_animations_(0),
            last_resize_animations_(0),
            ongoing_animations_(false),
            root_collapsing_(false)
    {
		// Always require a layout manager to operate the menu.
		assert(layout_manager_);

        // Create the root menu
        root_menu_ = new GroupNode(true, "RootNode", "", -20, 5);
        category_map_["Root"] = root_menu_;
//$ BEGIN_MOD $
//$ MOD_DESCRIPTION Not used until one module needs to put a widget in the scene and an option on the menu, so we do it later if needed $ 
		/*layout_manager_->AddCornerAnchor(root_menu_, Qt::TopLeftCorner, Qt::TopLeftCorner);
        connect(root_menu_, SIGNAL(NodeGroupClicked(GroupNode*, QParallelAnimationGroup*, QParallelAnimationGroup*)),
                SLOT(GroupNodeClicked(GroupNode*, QParallelAnimationGroup *, QParallelAnimationGroup *)));*/
//$ END_MOD $ 
    }

    MenuManager::~MenuManager()
    {
        SAFE_DELETE(root_menu_);
    }

    void MenuManager::AddMenuGroup(const QString &name, const QString &icon, qreal hgap, qreal vgap)
    {
		assert(layout_manager_);

		//$ BEGIN_MOD $
		//$ MOD_DESCRIPTION Someone needs to render something in the screen $ 
		layout_manager_->AddCornerAnchor(root_menu_, Qt::TopLeftCorner, Qt::TopLeftCorner);
        connect(root_menu_, SIGNAL(NodeGroupClicked(GroupNode*, QParallelAnimationGroup*, QParallelAnimationGroup*)),
                SLOT(GroupNodeClicked(GroupNode*, QParallelAnimationGroup *, QParallelAnimationGroup *)));
		//$ END_MOD $   

		///\todo Remove this hack at some point.
        QString hack_icon;
        QString base_url = "./data/ui/images/menus/";
        if (name == "Server Tools")
            hack_icon = base_url + "edbutton_SRVRTOOLS_icon.png";
        else if (name == "World Tools")
            hack_icon = base_url + "edbutton_WRLDTOOLS_icon.png";
        else
            hack_icon = defaultGroupIcon;

        GroupNode *group_node = new GroupNode(false, name, hack_icon, hgap, vgap);
        root_menu_->AddChildNode(group_node);
        category_map_[name] = group_node;
        layout_manager_->AddItemToScene(group_node);

        connect(group_node, SIGNAL(NodeGroupClicked(GroupNode*, QParallelAnimationGroup*, QParallelAnimationGroup*)),
                SLOT(GroupNodeClicked(GroupNode*, QParallelAnimationGroup *, QParallelAnimationGroup *)));

        Sort(); 
    }

    void MenuManager::AddMenuItem(QWidget *widget, const QString &name, const QString &category, const QString &icon)
    {
//$ BEGIN_MOD $
		if(!root_menu_->isVisible())
			root_menu_->setVisible(true);
//$ END_MOD $
        ActionNode *child_node = new ActionNode(name, icon);
        ///\todo hack protection for menu crashing when root having more than 5 items.
        /// Remove when Qt 4.7 supposedly fixes this.
        if ((category.isEmpty() || category == "Root") && root_menu_->ChildCount() >= 5)
        {
            if (category_map_.contains("Edit"))
            {
                category_map_["Edit"]->AddChildNode(child_node);
            }
            else
            {
                //LogError("Aborted adding a child node to menu, as Qt < 4.7 root menu hack was in place, and the fallback category 'Edit' was not found");
            }
        }
        else if (category.isEmpty())
        {
			if (category_map_.contains("Root"))
				category_map_["Root"]->AddChildNode(child_node);
			else
				LogError("CoreUi::MenuManager: Category map doesn't contain a 'Root' element!");

        }
        else if (category_map_.contains(category))
        {
            category_map_[category]->AddChildNode(child_node);
        }
        else
        {
            AddMenuGroup(category);
			assert(category_map_.contains(category));
            category_map_[category]->AddChildNode(child_node);
        }

        controller_map_[child_node->GetID()] = widget;
        connect(child_node, SIGNAL(ActionButtonClicked(const QUuid&)), SLOT(ActionNodeClicked(const QUuid&)));
        layout_manager_->AddItemToScene(child_node);

		Sort();
    }

    void MenuManager::RemoveMenuItem(QWidget *controlled_widget)
    {
        QUuid remove_id = controller_map_.key(controlled_widget);
        if (remove_id.isNull())
            return;
/*
        MenuNode *recovered_node = 0;
        if (category_map_.contains(category))
            recovered_node =  category_map_[category]->RemoveChildNode(remove_id);
        else
            return;
*/
        MenuNode *recovered_node = 0;
        QMapIterator<QString, GroupNode*> it(category_map_);
        while(it.hasNext())
        {
            recovered_node = it.next().value()->RemoveChildNode(remove_id);
			if (recovered_node){
//$ BEGIN_MOD $
//$ MOD_DESCRIPTION Removes the category if it is empty $ 
				if(it.value()->GetChildNodeList().count()==0){
					root_menu_->RemoveChildNode(it.value()->GetID());
					layout_manager_->RemoveItemFromScene(category_map_[it.key()]);
					category_map_.remove(it.key());
				}
//$ END_MOD $
                break;
			}
        }

        if (recovered_node)
        {
            controller_map_.remove(remove_id);
            disconnect(recovered_node, SIGNAL(ActionButtonClicked(const QUuid&)), this, SLOT(ActionNodeClicked(const QUuid&)));
            layout_manager_->RemoveItemFromScene(recovered_node);
        }
//$ BEGIN_MOD $
//$ MOD_DESCRIPTION Removes the Root Menu if it is empty $ 
		if(controller_map_.empty())
			root_menu_->setVisible(false);
		else
			Sort();
//$ END_MOD $
    }

    void MenuManager::ActionNodeClicked(const QUuid &id)
    {
        if (!controller_map_.contains(id))
            return;

        QWidget *controlled_widget = controller_map_[id];
        if (!controlled_widget)
            return;

        UiProxyWidget *naali_proxy = dynamic_cast<UiProxyWidget *>(controlled_widget);
        if (!naali_proxy)
        {
            if (!controlled_widget->isVisible())
                controlled_widget->show();
            else
                controlled_widget->hide();
        }
        else
        {
            if (!naali_proxy->isVisible())
                naali_proxy->show();
            else
                naali_proxy->AnimatedHide();
        }
    }

    void MenuManager::GroupNodeClicked(GroupNode *node, QParallelAnimationGroup *move_animations, QParallelAnimationGroup *size_animations)
    {
        if (ongoing_animations_)
            return;

        QSequentialAnimationGroup *revert_group = new QSequentialAnimationGroup(this);
        QSequentialAnimationGroup *sequential_move_animations_ = new QSequentialAnimationGroup(this);
        sequential_move_animations_->setDirection(QAbstractAnimation::Backward);

        // Make a sequential animation of currently expanding nodes
        // Only revert into the same tree depth as the clicked item
        int click_tree_depth = node->GetTreeDepth();
        foreach (GroupNode *node, expanded_nodes_)
        {
            if (node->GetTreeDepth() >= click_tree_depth && node->IsExpanded())
            {
                QParallelAnimationGroup *node_move_anim = node->GetMoveAnimations();
                sequential_move_animations_->addAnimation(node_move_anim);
                expanded_nodes_.removeOne(node);
            }
        }
        revert_group->addAnimation(sequential_move_animations_);
        if (last_resize_animations_)
            revert_group->addAnimation(last_resize_animations_);

        // New animations
        next_move_animations_ = move_animations;
        last_resize_animations_ = size_animations;
        expanded_nodes_.append(node);

        connect(move_animations, SIGNAL( finished() ), size_animations, SLOT( start() ));
        connect(size_animations, SIGNAL( finished() ), SLOT( AdjustTreeOpacity() ));
        // Normal item, we are expanding now, hook up move animations after revert is done
        if (!node->IsExpanded())
        {
            connect(revert_group, SIGNAL(finished()), SLOT(RevertAnimationsFinished()));
        }
        // Clicked item is root, and its collapsing
        else if (!node->parent())
        {
            connect(revert_group, SIGNAL(finished()), SLOT(RevertAnimationsFinished()));
            root_collapsing_ = true;
            expanded_nodes_.clear();
            expanded_nodes_.append(node);
        }
        // Else its a normal item collapsing, lets remove it from tracking map of expanded items
        else
        {
            expanded_nodes_.removeOne(node);
            last_resize_animations_ = 0;

            GroupNode* group_node = dynamic_cast<GroupNode*>(node->parent());
            if (group_node)
            {
                group_node->AdjustNode(QAbstractAnimation::Forward);
                last_resize_animations_ = group_node->GetResizeAnimations();
            }
        }

        revert_group->setDirection(QAbstractAnimation::Backward);
        revert_group->start();
        ongoing_animations_ = true;
    }

    void MenuManager::RevertAnimationsFinished()
    {
        if (root_collapsing_)
        {
            next_move_animations_ = 0;
            last_resize_animations_ = 0;
            root_collapsing_ = false;
            ongoing_animations_ = false;
            expanded_nodes_.at(0)->GetMoveAnimations()->stop();
            expanded_nodes_.at(0)->GetResizeAnimations()->stop();
            expanded_nodes_.clear();
        }

        if (next_move_animations_)
        {
            connect(next_move_animations_, SIGNAL( finished() ), SLOT( MoveAnimationsFinished() ));
            next_move_animations_->start();
        }
    }

    void MenuManager::MoveAnimationsFinished()
    {
        ongoing_animations_ = false;
    }

    void MenuManager::AdjustTreeOpacity()
    {
        if (expanded_nodes_.count() <= 1)
            return;

        GroupNode *process_layer = expanded_nodes_.last();
		if (!process_layer)
			return;
        int depth = process_layer->GetTreeDepth();
        bool level_found = false;

        process_layer = root_menu_;
        while (!level_found)
        {
            foreach(MenuNode *node, process_layer->GetChildNodeList())
            {
                if (node->GetTreeDepth() == depth)
                {
                    node->setOpacity(0.65);
                    level_found = true;
                }
                else if (dynamic_cast<GroupNode*>(node))
                {
                    process_layer = dynamic_cast<GroupNode*>(node);
                    break;
                }

                if (dynamic_cast<GroupNode*>(node) && dynamic_cast<GroupNode*>(node)->IsExpanded())
                    node->setOpacity(1);
            }
        }
    }

    void MenuManager::Sort()
    {
        root_menu_->Sort();
		if (layout_manager_->GetScene())
			layout_manager_->GetScene()->update();
    }
}
