// Simple client_profile editor.
// Open settings_provider with default settings (or a specified settings file),
// display current data and allow editing profile fields using Qt4 gui.
// Save profile back.

// Example profile instantiation:
    settings_provider::set_organization_name("atta");
    settings_provider::set_organization_domain("net.atta-metta");
    settings_provider::set_application_name("opus-streaming");
    auto settings = settings_provider::instance();

// Example profile fields:

    // This host identity (EID)
    // Generated via ssu::identity class, e.g. identity::generate()
    settings->set("id", host_identity_.id().id());
    // Private key for this identity. Necessary to prove identity ownership and set up
    // encrypted sessions.
    settings->set("key", host_identity_.private_key());
    // There should some sort of GUI to let user generate a new identity with given specification
    // (RSA or DSA etc)

    settings->set("port", stream_protocol::default_port);

    uia::routing::client_profile client;
    client.set_host_name("aramaki.local");
    client.set_owner_name("Berkus");
    client.set_city("Tallinn");
    client.set_region("Harju");
    client.set_country("Estonia");

    settings->set("profile", client.enflurry()); // Set as single serialized byte_array blob

    vector<string> regservers = settings->get("regservers");
    // Regservers list as ipv4, ipv4:port, [ipv6], [ipv6]:port, srvname, srvname:srvport
