/*
 *  PineconeMC Offline - Minecraft Launcher
 *  Copyright (C) 2026 PineconeMC Offline Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
package org.pineconemc.forgeprep;

import java.io.File;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;

/**
 * Runs the Forge/NeoForge installer post-processors through ForgeWrapper, the way ForgeWrapper's own Main does before
 * starting the game, but without starting it. Used when exporting an offline bundle, so the processor outputs can be
 * shipped and the first offline launch finds them already done.
 */
public final class Main {
    public static void main(String[] args) throws Exception {
        if (args.length != 4) {
            System.out.println("Usage: java -jar pinecone-forge-prep.jar <ForgeWrapper jar> <installer jar> <libraries dir> <minecraft jar>");
            System.exit(2);
        }
        File wrapper = new File(args[0]);
        File installer = new File(args[1]);
        File libraries = new File(args[2]);
        File minecraft = new File(args[3]);
        for (File f : new File[] { wrapper, installer, minecraft }) {
            if (!f.isFile()) {
                System.out.println("Missing file: " + f);
                System.exit(3);
            }
        }
        URLClassLoader loader = new URLClassLoader(new URL[] { wrapper.toURI().toURL(), installer.toURI().toURL() }, platformLoader());
        Class<?> installerClass = loader.loadClass("io.github.zekerzhayard.forgewrapper.installer.Installer");
        installerClass.getMethod("getData", File.class).invoke(null, libraries);
        Method install = installerClass.getMethod("install", File.class, File.class, File.class);
        boolean ok = (Boolean) install.invoke(null, libraries, minecraft, installer);
        System.out.println(ok ? "FORGE_PREP_OK" : "FORGE_PREP_FAILED");
        System.exit(ok ? 0 : 1);
    }

    // The same parent ForgeWrapper uses: the platform class loader on Java 9+, the extension loader on Java 8.
    private static ClassLoader platformLoader() {
        try {
            return (ClassLoader) ClassLoader.class.getMethod("getPlatformClassLoader").invoke(null);
        } catch (Exception e) {
            return ClassLoader.getSystemClassLoader().getParent();
        }
    }
}
