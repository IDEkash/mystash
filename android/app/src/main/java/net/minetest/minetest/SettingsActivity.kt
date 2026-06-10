package net.minetest.minetest

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.SwitchCompat
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView

class SettingsActivity : AppCompatActivity() {

    private lateinit var rvMods: RecyclerView
    private lateinit var tvNoMods: TextView
    private lateinit var restartBanner: View
    private lateinit var btnRestart: Button
    private lateinit var btnImport: Button
    private lateinit var adapter: ModAdapter

    private var needsRestart = false

    private val importLauncher = registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
        uri?.let {
            if (NativeModManager.importMod(this, it)) {
                refreshModList()
                showRestartBanner()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_settings)

        rvMods = findViewById(R.id.rvMods)
        tvNoMods = findViewById(R.id.tvNoMods)
        restartBanner = findViewById(R.id.restartBanner)
        btnRestart = findViewById(R.id.btnRestart)
        btnImport = findViewById(R.id.btnImport)

        rvMods.layoutManager = LinearLayoutManager(this)
        adapter = ModAdapter(mutableListOf(),
            onToggle = { mod, enabled ->
                NativeModManager.setModEnabled(this, mod.name, enabled)
                showRestartBanner()
            },
            onDelete = { mod ->
                NativeModManager.deleteMod(this, mod.name)
                refreshModList()
                showRestartBanner()
            }
        )
        rvMods.adapter = adapter

        btnImport.setOnClickListener {
            importLauncher.launch("*/*")
        }

        btnRestart.setOnClickListener {
            restartApp(this)
        }

        refreshModList()
    }

    private fun refreshModList() {
        val mods = NativeModManager.getInstalledMods(this)
        adapter.updateMods(mods)
        tvNoMods.visibility = if (mods.isEmpty()) View.VISIBLE else View.GONE
    }

    private fun showRestartBanner() {
        needsRestart = true
        restartBanner.visibility = View.VISIBLE
    }

    private fun restartApp(context: Context) {
        val intent = context.packageManager.getLaunchIntentForPackage(context.packageName)
        intent?.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
        context.startActivity(intent)
        android.os.Process.killProcess(android.os.Process.myPid())
    }

    private class ModAdapter(
        private var mods: MutableList<NativeMod>,
        private val onToggle: (NativeMod, Boolean) -> Unit,
        private val onDelete: (NativeMod) -> Unit
    ) : RecyclerView.Adapter<ModAdapter.ViewHolder>() {

        class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
            val tvName: TextView = view.findViewById(R.id.tvModName)
            val swEnable: SwitchCompat = view.findViewById(R.id.swEnable)
            val btnDelete: Button = view.findViewById(R.id.btnDelete)
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val view = LayoutInflater.from(parent.context).inflate(R.layout.item_native_mod, parent, false)
            return ViewHolder(view)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            val mod = mods[position]
            holder.tvName.text = mod.name

            // Unset listener before setting isChecked to avoid triggering listener on recycled views
            holder.swEnable.setOnCheckedChangeListener(null)
            holder.swEnable.isChecked = mod.enabled

            holder.swEnable.setOnCheckedChangeListener { _, isChecked ->
                onToggle(mod, isChecked)
            }
            holder.btnDelete.setOnClickListener {
                onDelete(mod)
            }
        }

        override fun getItemCount() = mods.size

        fun updateMods(newMods: List<NativeMod>) {
            mods.clear()
            mods.addAll(newMods)
            notifyDataSetChanged()
        }
    }
}
