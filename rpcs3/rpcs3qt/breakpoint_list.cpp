#include "breakpoint_list.h"
#include "breakpoint_handler.h"

#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUThread.h"

#include <QMenu>

constexpr auto qstr = QString::fromStdString;

breakpoint_list::breakpoint_list(QWidget* parent, breakpoint_handler* handler) : QListWidget(parent), m_breakpoint_handler(handler)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	// connects
	connect(this, &QListWidget::itemDoubleClicked, this, &breakpoint_list::OnBreakpointListDoubleClicked);
	connect(this, &QListWidget::customContextMenuRequested, this, &breakpoint_list::OnBreakpointListRightClicked);
}

/**
* It's unfortunate I need a method like this to sync these.  Should ponder a cleaner way to do this.
*/
void breakpoint_list::UpdateCPUData(std::weak_ptr<cpu_thread> cpu, std::shared_ptr<CPUDisAsm> disasm)
{
	this->cpu = cpu;
	m_disasm = disasm;
}

void breakpoint_list::ClearBreakpoints()
{
	while (count())
	{
		auto* currentItem = takeItem(0);
		u32 loc = currentItem->data(Qt::UserRole).value<u32>();
		m_breakpoint_handler->RemoveBreakpoint(loc, breakpoint_type::bp_execute);
		delete currentItem;
	}
}

void breakpoint_list::RemoveBreakpoint(u32 addr)
{
	m_breakpoint_handler->RemoveBreakpoint(addr, breakpoint_type::bp_execute);

	for (int i = 0; i < count(); i++)
	{
		QListWidgetItem* currentItem = item(i);

		if (currentItem->data(Qt::UserRole).value<u32>() == addr)
		{
			delete takeItem(i);
			break;
		}
	}

	Q_EMIT RequestShowAddress(addr);
}

void breakpoint_list::AddBreakpoint(u32 pc, bs_t<breakpoint_type> type)
{
	m_breakpoint_handler->AddBreakpoint(pc, type);

	const auto cpu = this->cpu.lock();
	const auto cpu_offset = cpu->id_type() == 2 ? static_cast<spu_thread&>(*cpu).ls : vm::g_sudo_addr;
	m_disasm->offset = cpu_offset;

	QString breakpointItemText;

	if (type == breakpoint_type::bp_execute)
	{
		m_disasm->disasm(m_disasm->dump_pc = pc);

		breakpointItemText = qstr(m_disasm->last_opcode);

		breakpointItemText.remove(10, 13);
	}

	if (type == breakpoint_type::bp_mread)	breakpointItemText = QString("BPMR:  0x%1").arg(pc, 8, 16, QChar('0'));
	if (type == breakpoint_type::bp_mwrite)	breakpointItemText = QString("BPMW:  0x%1").arg(pc, 8, 16, QChar('0'));
	if (type == (breakpoint_type::bp_mread + breakpoint_type::bp_mwrite)) breakpointItemText = QString("BPMRW: 0x%1").arg(pc, 8, 16, QChar('0'));

	QListWidgetItem* breakpointItem = new QListWidgetItem(breakpointItemText);
	breakpointItem->setForeground(m_text_color_bp);
	breakpointItem->setBackground(m_color_bp);
	QVariant pcVariant;
	pcVariant.setValue(pc);
	breakpointItem->setData(Qt::UserRole, pcVariant);
	addItem(breakpointItem);

	Q_EMIT RequestShowAddress(pc);
}

/**
* If breakpoint exists, we remove it, else add new one.  Yeah, it'd be nicer from a code logic to have it be set/reset.  But, that logic has to happen somewhere anyhow.
*/
void breakpoint_list::HandleBreakpointRequest(u32 loc)
{
	if (m_breakpoint_handler->HasBreakpoint(loc, breakpoint_type::bp_execute))
	{
		RemoveBreakpoint(loc);
	}
	else
	{
		const auto cpu = this->cpu.lock();

		if (cpu->id_type() == 1 && vm::check_addr(loc, vm::page_allocated | vm::page_executable))
		{
			AddBreakpoint(loc, breakpoint_type::bp_execute);
		}
	}
}

void breakpoint_list::OnBreakpointListDoubleClicked()
{
	u32 address = currentItem()->data(Qt::UserRole).value<u32>();
	Q_EMIT RequestShowAddress(address);
}

void breakpoint_list::OnBreakpointListRightClicked(const QPoint &pos)
{
	QMenu* menu = new QMenu();

	if (selectedItems().count() == 1)
	{
		menu->addAction(tr("Rename"));
		menu->addSeparator();
	}

	if (selectedItems().count() >= 1)
	{
		QAction* m_breakpoint_list_delete = new QAction(tr("Delete"), this);
		m_breakpoint_list_delete->setShortcut(Qt::Key_Delete);
		m_breakpoint_list_delete->setShortcutContext(Qt::WidgetShortcut);
		addAction(m_breakpoint_list_delete);
		connect(m_breakpoint_list_delete, &QAction::triggered, this, &breakpoint_list::OnBreakpointListDelete);

		menu->addAction(m_breakpoint_list_delete);
		menu->addSeparator();
	}

	QAction* m_addbp = new QAction(tr("Add Breakpoint"), this);
	addAction(m_addbp);
	connect(m_addbp, &QAction::triggered, this, &breakpoint_list::ShowAddBreakpointWindow);
	menu->addAction(m_addbp);

	QAction* selectedItem = menu->exec(viewport()->mapToGlobal(pos));
	if (selectedItem)
	{
		if (selectedItem->text() == "Rename")
		{
			QListWidgetItem* currentItem = selectedItems().at(0);
			currentItem->setFlags(currentItem->flags() | Qt::ItemIsEditable);
			editItem(currentItem);
		}
	}

}

void breakpoint_list::OnBreakpointListDelete()
{
	int selectedCount = selectedItems().count();

	for (int i = selectedCount - 1; i >= 0; i--)
	{
		RemoveBreakpoint(item(i)->data(Qt::UserRole).value<u32>());
	}
}

void breakpoint_list::ShowAddBreakpointWindow()
{
	QDialog* diag = new QDialog(this);

	diag->setWindowTitle(tr("Add a breakpoint"));
	diag->setModal(true);

	QVBoxLayout *vbox_panel = new QVBoxLayout();

	QHBoxLayout *hbox_top = new QHBoxLayout();
	QLabel *l_address = new QLabel(tr("Address"));
	QLineEdit *t_address = new QLineEdit();
	t_address->setPlaceholderText("Address here");
	t_address->setFocus();

	hbox_top->addWidget(l_address);
	hbox_top->addWidget(t_address);
	vbox_panel->addLayout(hbox_top);

	QHBoxLayout *hbox_bot = new QHBoxLayout();
	QComboBox *co_bptype = new QComboBox(this);
	QStringList breakpoint_types;
	breakpoint_types << "Memory Read" << "Memory Write" << "Memory Read&Write" << "Execution";
	co_bptype->addItems(breakpoint_types);

	hbox_bot->addWidget(co_bptype);
	vbox_panel->addLayout(hbox_bot);


	QHBoxLayout *hbox_buttons = new QHBoxLayout();
	QPushButton* b_cancel = new QPushButton(tr("Cancel"));
	QPushButton* b_addbp = new QPushButton(tr("Add"));

	hbox_buttons->addWidget(b_cancel);
	hbox_buttons->addWidget(b_addbp);
	vbox_panel->addLayout(hbox_buttons);

	diag->setLayout(vbox_panel);

	connect(b_cancel, &QAbstractButton::clicked, diag, &QDialog::reject);
	connect(b_addbp, &QAbstractButton::clicked, diag, &QDialog::accept);

	diag->move(QCursor::pos());

	if (diag->exec() == QDialog::Accepted)
	{
		if (!t_address->text().isEmpty())
		{
			u32 address = std::stoul(t_address->text().toStdString(), nullptr, 16);
			bs_t<breakpoint_type> bp_t;
			switch(co_bptype->currentIndex())
			{
			case 0:
				bp_t = breakpoint_type::bp_mread;
				break;
			case 1:
				bp_t = breakpoint_type::bp_mwrite;
				break;
			case 2:
				bp_t = breakpoint_type::bp_mread + breakpoint_type::bp_mwrite;
				break;
			case 3:
				bp_t = breakpoint_type::bp_execute;
				break;
			default:
				bp_t = {};
				break;
			}

			if (bp_t) AddBreakpoint(address, bp_t);
		}
	}

	diag->deleteLater();
}
